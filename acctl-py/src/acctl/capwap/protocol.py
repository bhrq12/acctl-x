#!/usr/bin/python3
"""
CAPWAP Protocol Implementation
Handles CAPWAP message parsing, construction and processing.
"""

import struct
import socket
from typing import Dict, List, Any, Optional
from .messages import (
    CAPWAP_HEADER_LENGTH,
    CAPWAP_MESSAGE_TYPES,
    CAPWAP_ELEMENT_TYPES,
    CAPWAP_STATUS_CODES,
    CAPWAP_VERSION,
    CAPWAP_CONTROL_PORT
)
from ..logger import logger


class CAPWAPProtocol:
    """CAPWAP protocol handler."""
    
    def __init__(self):
        self.socket = None
        self.listen_port = CAPWAP_CONTROL_PORT
    
    def start(self, port: int = CAPWAP_CONTROL_PORT):
        """Start CAPWAP listener."""
        self.listen_port = port
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.socket.bind(('0.0.0.0', port))
        logger.info(f"CAPWAP listener started on port {port}")
    
    def stop(self):
        """Stop CAPWAP listener."""
        if self.socket:
            self.socket.close()
            logger.info("CAPWAP listener stopped")
    
    def recv_message(self, buffer_size: int = 4096) -> Optional[Dict[str, Any]]:
        """Receive and parse CAPWAP message."""
        if not self.socket:
            return None
        
        try:
            data, addr = self.socket.recvfrom(buffer_size)
            if len(data) < CAPWAP_HEADER_LENGTH:
                logger.warn("Invalid CAPWAP message: too short")
                return None
            
            message = self._parse_message(data)
            message['source'] = addr
            return message
        except Exception as e:
            logger.error(f"Error receiving CAPWAP message: {e}")
            return None
    
    def _parse_message(self, data: bytes) -> Dict[str, Any]:
        """Parse CAPWAP message from raw bytes."""
        message = {
            'version': None,
            'type': None,
            'length': None,
            'flags': None,
            'session_id': None,
            'elements': []
        }
        
        # Parse header (12 bytes)
        # Byte 0-1: Version + Flags + Type
        version = (data[0] >> 4) & 0x0F
        flags = data[0] & 0x0F
        msg_type = data[1]
        
        # Byte 2-3: Length
        length = struct.unpack('!H', data[2:4])[0]
        
        # Byte 4-11: Session ID
        session_id = struct.unpack('!Q', data[4:12])[0]
        
        message['version'] = version
        message['type'] = msg_type
        message['type_name'] = CAPWAP_MESSAGE_TYPES.get(msg_type, f'UNKNOWN_{msg_type}')
        message['length'] = length
        message['flags'] = flags
        message['session_id'] = session_id
        
        # Parse elements
        offset = CAPWAP_HEADER_LENGTH
        while offset < len(data):
            if offset + 4 > len(data):
                break
            
            # Element header: type (2 bytes), length (2 bytes)
            elem_type = struct.unpack('!H', data[offset:offset+2])[0]
            elem_length = struct.unpack('!H', data[offset+2:offset+4])[0]
            
            if offset + 4 + elem_length > len(data):
                break
            
            elem_data = data[offset+4:offset+4+elem_length]
            element = self._parse_element(elem_type, elem_data)
            message['elements'].append(element)
            
            offset += 4 + elem_length
        
        return message
    
    def _parse_element(self, elem_type: int, elem_data: bytes) -> Dict[str, Any]:
        """Parse individual CAPWAP element."""
        element = {
            'type': elem_type,
            'type_name': CAPWAP_ELEMENT_TYPES.get(elem_type, f'UNKNOWN_{elem_type}'),
            'data': elem_data
        }
        
        # Try to parse common elements
        if elem_type == 7:  # AP_MAC
            element['value'] = ':'.join(f'{b:02x}' for b in elem_data)
        elif elem_type == 2:  # AC_NAME
            element['value'] = elem_data.decode('utf-8', errors='ignore').strip('\x00')
        elif elem_type == 5:  # AP_NAME
            element['value'] = elem_data.decode('utf-8', errors='ignore').strip('\x00')
        elif elem_type == 24:  # IPV4_ADDRESS
            element['value'] = socket.inet_ntoa(elem_data)
        elif elem_type == 33:  # STATUS_CODE
            status_code = struct.unpack('!H', elem_data)[0]
            element['value'] = {
                'code': status_code,
                'message': CAPWAP_STATUS_CODES.get(status_code, 'Unknown')
            }
        elif elem_type == 19:  # ECHO_DATA
            element['value'] = elem_data.hex()
        
        return element
    
    def build_message(self, msg_type: int, session_id: int = 0, 
                      elements: List[Dict[str, Any]] = None) -> bytes:
        """Build CAPWAP message from components."""
        elements = elements or []
        
        # Build elements data
        elements_data = b''
        for elem in elements:
            elem_type = elem['type']
            elem_data = elem.get('data', b'')
            elem_length = len(elem_data)
            
            # Element header: type (2 bytes), length (2 bytes)
            elements_data += struct.pack('!HH', elem_type, elem_length)
            elements_data += elem_data
        
        # Calculate total length
        total_length = CAPWAP_HEADER_LENGTH + len(elements_data)
        
        # Build header
        # Version (4 bits) + Flags (4 bits)
        version_flags = (CAPWAP_VERSION << 4) & 0xF0
        
        # Message type (1 byte)
        # Length (2 bytes)
        # Session ID (8 bytes)
        
        header = struct.pack(
            '!BBHQ',
            version_flags,
            msg_type,
            total_length,
            session_id
        )
        
        return header + elements_data
    
    def build_discover_response(self, ac_name: str, ac_ip: str) -> bytes:
        """Build DISCOVER_RESPONSE message."""
        elements = [
            {
                'type': 2,  # AC_NAME
                'data': ac_name.encode('utf-8') + b'\x00'
            },
            {
                'type': 24,  # IPV4_ADDRESS
                'data': socket.inet_aton(ac_ip)
            },
            {
                'type': 32,  # SESSION_TIMEOUT
                'data': struct.pack('!I', 300)  # 5 minutes
            }
        ]
        
        return self.build_message(2, 0, elements)  # DISCOVER_RESPONSE
    
    def build_join_response(self, session_id: int, status_code: int = 0) -> bytes:
        """Build JOIN_RESPONSE message."""
        elements = [
            {
                'type': 33,  # STATUS_CODE
                'data': struct.pack('!H', status_code)
            }
        ]
        
        return self.build_message(4, session_id, elements)  # JOIN_RESPONSE
    
    def send_message(self, message: bytes, addr: tuple):
        """Send CAPWAP message to address."""
        if self.socket:
            self.socket.sendto(message, addr)
    
    def send_discover_response(self, addr: tuple, ac_name: str = 'OpenWrt-AC'):
        """Send DISCOVER_RESPONSE to AP."""
        # Get local IP address
        try:
            s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            s.connect((addr[0], 1))
            local_ip = s.getsockname()[0]
            s.close()
        except Exception:
            local_ip = '127.0.0.1'
        
        response = self.build_discover_response(ac_name, local_ip)
        self.send_message(response, addr)
        logger.info(f"Sent DISCOVER_RESPONSE to {addr[0]}:{addr[1]}")
