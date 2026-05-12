#!/usr/bin/python3
"""
AP Manager Module
Manages AP device discovery, registration, status monitoring and configuration.
"""

import time
import threading
from typing import Dict, List, Optional, Any
from datetime import datetime
from .db import db_manager
from .config import config_manager
from .security import security_manager
from .capwap.protocol import CAPWAPProtocol
from .logger import logger


class APInfo:
    """Data structure for AP information."""
    
    def __init__(self, mac: str, **kwargs):
        self.mac = mac
        self.name = kwargs.get('name', '')
        self.model = kwargs.get('model', '')
        self.firmware_version = kwargs.get('firmware_version', '')
        self.ip_address = kwargs.get('ip_address', '')
        self.status = kwargs.get('status', 'offline')
        self.uptime = kwargs.get('uptime', 0)
        self.clients = kwargs.get('clients', 0)
        self.last_seen = kwargs.get('last_seen', datetime.now())
        self.registered_at = kwargs.get('registered_at', datetime.now())
    
    def to_dict(self) -> Dict[str, Any]:
        """Convert to dictionary."""
        return {
            'mac': self.mac,
            'name': self.name,
            'model': self.model,
            'firmware_version': self.firmware_version,
            'ip_address': self.ip_address,
            'status': self.status,
            'uptime': self.uptime,
            'clients': self.clients,
            'last_seen': self.last_seen.isoformat() if hasattr(self.last_seen, 'isoformat') else str(self.last_seen),
            'registered_at': self.registered_at.isoformat() if hasattr(self.registered_at, 'isoformat') else str(self.registered_at)
        }


class APManager:
    """Manages AP devices and their lifecycle."""
    
    def __init__(self):
        self.aps: Dict[str, APInfo] = {}
        self.lock = threading.Lock()
        self.capwap_protocol = CAPWAPProtocol()
        self.running = False
        self._load_from_db()
    
    def _load_from_db(self):
        """Load AP information from database."""
        aps = db_manager.get_all_aps()
        for ap in aps:
            self.aps[ap['mac']] = APInfo(**ap)
    
    def start(self):
        """Start AP manager."""
        self.running = True
        
        # Start CAPWAP listener
        network_config = config_manager.get_network_config()
        capwap_port = network_config.get('capwap_port', 5246)
        
        try:
            self.capwap_protocol.start(capwap_port)
            
            # Start message processing thread
            self.thread = threading.Thread(target=self._process_messages, daemon=True)
            self.thread.start()
            
            # Start status monitoring thread
            self.monitor_thread = threading.Thread(target=self._monitor_status, daemon=True)
            self.monitor_thread.start()
            
            logger.info("AP Manager started")
        except Exception as e:
            logger.error(f"Failed to start AP Manager: {e}")
            self.running = False
    
    def stop(self):
        """Stop AP manager."""
        self.running = False
        self.capwap_protocol.stop()
        logger.info("AP Manager stopped")
    
    def _process_messages(self):
        """Process incoming CAPWAP messages."""
        while self.running:
            message = self.capwap_protocol.recv_message()
            if message:
                self._handle_message(message)
            time.sleep(0.01)
    
    def _handle_message(self, message: Dict[str, Any]):
        """Handle CAPWAP message based on type."""
        msg_type = message['type']
        source = message['source']
        
        logger.debug(f"Received CAPWAP message type {message['type_name']} from {source}")
        
        if msg_type == 1:  # DISCOVER_REQUEST
            self._handle_discover_request(message)
        elif msg_type == 3:  # JOIN_REQUEST
            self._handle_join_request(message)
        elif msg_type == 5:  # CONFIGURATION_REQUEST
            self._handle_configuration_request(message)
        elif msg_type == 12:  # ECHO_REQUEST
            self._handle_echo_request(message)
    
    def _handle_discover_request(self, message: Dict[str, Any]):
        """Handle AP discovery request."""
        source = message['source']
        ac_name = config_manager.get_ac_config().get('name', 'OpenWrt-AC-Py')
        
        # Extract AP MAC from message elements
        ap_mac = None
        for elem in message['elements']:
            if elem['type'] == 7:  # AP_MAC
                ap_mac = elem.get('value', '')
                break
        
        if ap_mac:
            logger.info(f"AP discovery request from {ap_mac} ({source[0]})")
        
        # Send discovery response
        self.capwap_protocol.send_discover_response(source, ac_name)
    
    def _handle_join_request(self, message: Dict[str, Any]):
        """Handle AP join request."""
        source = message['source']
        
        # Extract AP info from elements
        ap_info = {
            'ip_address': source[0],
            'status': 'online'
        }
        
        for elem in message['elements']:
            if elem['type'] == 7:  # AP_MAC
                ap_info['mac'] = elem.get('value', '')
            elif elem['type'] == 5:  # AP_NAME
                ap_info['name'] = elem.get('value', '')
        
        if 'mac' in ap_info:
            self.register_ap(ap_info['mac'], ap_info)
            
            # Send join response
            response = self.capwap_protocol.build_join_response(message['session_id'], 0)
            self.capwap_protocol.send_message(response, source)
            
            logger.info(f"AP {ap_info['mac']} joined from {source[0]}")
    
    def _handle_configuration_request(self, message: Dict[str, Any]):
        """Handle AP configuration request."""
        source = message['source']
        
        # Extract AP MAC
        ap_mac = None
        for elem in message['elements']:
            if elem['type'] == 7:  # AP_MAC
                ap_mac = elem.get('value', '')
                break
        
        if ap_mac:
            # Get AP configuration from database
            config = db_manager.get_ap_config(ap_mac)
            
            # Send configuration response
            # This is a simplified implementation
            response = self.capwap_protocol.build_message(
                6,  # CONFIGURATION_RESPONSE
                message['session_id'],
                []
            )
            self.capwap_protocol.send_message(response, source)
    
    def _handle_echo_request(self, message: Dict[str, Any]):
        """Handle echo request (keep-alive)."""
        source = message['source']
        
        # Extract echo data
        echo_data = b''
        for elem in message['elements']:
            if elem['type'] == 19:  # ECHO_DATA
                echo_data = elem.get('data', b'')
                break
        
        # Send echo response with same data
        response = self.capwap_protocol.build_message(
            13,  # ECHO_RESPONSE
            message['session_id'],
            [{'type': 19, 'data': echo_data}]
        )
        self.capwap_protocol.send_message(response, source)
        
        # Update AP last seen time
        for elem in message['elements']:
            if elem['type'] == 7:  # AP_MAC
                ap_mac = elem.get('value', '')
                if ap_mac in self.aps:
                    self.aps[ap_mac].last_seen = datetime.now()
                    self.aps[ap_mac].status = 'online'
                    db_manager.update_ap_status(ap_mac, 'online')
                break
    
    def _monitor_status(self):
        """Monitor AP status and mark offline if no heartbeat."""
        while self.running:
            now = datetime.now()
            with self.lock:
                for mac, ap in list(self.aps.items()):
                    # Check if last seen was more than 30 seconds ago
                    last_seen = ap.last_seen
                    if isinstance(last_seen, str):
                        last_seen = datetime.fromisoformat(last_seen)
                    
                    time_diff = (now - last_seen).total_seconds()
                    
                    if time_diff > 30 and ap.status == 'online':
                        ap.status = 'offline'
                        db_manager.update_ap_status(mac, 'offline')
                        logger.warn(f"AP {mac} marked as offline")
            
            time.sleep(10)
    
    def register_ap(self, mac: str, info: Dict[str, Any]):
        """Register or update AP information."""
        with self.lock:
            if mac in self.aps:
                ap = self.aps[mac]
                ap.name = info.get('name', ap.name)
                ap.model = info.get('model', ap.model)
                ap.firmware_version = info.get('firmware_version', ap.firmware_version)
                ap.ip_address = info.get('ip_address', ap.ip_address)
                ap.status = info.get('status', ap.status)
                ap.last_seen = datetime.now()
            else:
                info['mac'] = mac
                info['last_seen'] = datetime.now()
                info['registered_at'] = datetime.now()
                self.aps[mac] = APInfo(**info)
            
            # Save to database
            db_manager.add_ap(mac, self.aps[mac].to_dict())
    
    def deregister_ap(self, mac: str) -> bool:
        """Deregister AP."""
        with self.lock:
            if mac in self.aps:
                del self.aps[mac]
                db_manager.remove_ap(mac)
                logger.info(f"AP {mac} deregistered")
                return True
        return False
    
    def get_ap_status(self, mac: str) -> Optional[APInfo]:
        """Get AP status by MAC."""
        with self.lock:
            return self.aps.get(mac)
    
    def get_all_aps(self) -> List[APInfo]:
        """Get all registered APs."""
        with self.lock:
            return list(self.aps.values())
    
    def update_ap_config(self, mac: str, config: Dict[str, Any]) -> bool:
        """Update AP configuration."""
        with self.lock:
            if mac not in self.aps:
                return False
        
        db_manager.save_ap_config(mac, config)
        logger.info(f"Configuration updated for AP {mac}")
        return True
    
    def get_ap_config(self, mac: str) -> Dict[str, Any]:
        """Get AP configuration."""
        return db_manager.get_ap_config(mac)


# Global AP manager instance
ap_manager = APManager()
