#!/usr/bin/python3
"""
AC Controller Configuration Management Module
Handles UCI configuration parsing and runtime configuration management.
"""

import os
import json
from typing import Dict, Any, Optional

DEFAULT_CONFIG = {
    'global': {
        'enabled': True,
        'uuid': 'auto',
        'name': 'OpenWrt-AC-Py'
    },
    'network': {
        'listen_port': 8080,
        'capwap_port': 5246,
        'nic': ['br-lan']
    },
    'resource': {
        'ip_start': '192.168.1.200',
        'ip_end': '192.168.1.254',
        'ip_mask': '255.255.255.0'
    },
    'security': {
        'password': '',
        'dtls_enabled': False
    },
    'logging': {
        'level': 'info',
        'syslog': True
    }
}


class ConfigManager:
    """Manages AC Controller configuration."""
    
    def __init__(self, config_path: str = '/etc/config/acctl-py'):
        self.config_path = config_path
        self.config = DEFAULT_CONFIG.copy()
        self.load_config()
    
    def load_config(self):
        """Load configuration from UCI config file."""
        if os.path.exists(self.config_path):
            self._parse_uci_config()
        else:
            # Use defaults and save
            self.save_config()
    
    def _parse_uci_config(self):
        """Parse UCI format configuration file."""
        try:
            with open(self.config_path, 'r') as f:
                lines = f.readlines()
            
            current_section = None
            
            for line in lines:
                line = line.strip()
                if not line or line.startswith('#'):
                    continue
                
                if line.startswith('config'):
                    # Extract section type and name
                    parts = line.split()
                    if len(parts) >= 3:
                        current_section = parts[2].strip("'\"")
                    else:
                        current_section = parts[1].strip("'\"")
                    
                    if current_section not in self.config:
                        self.config[current_section] = {}
                
                elif line.startswith('option') and current_section:
                    # Parse option
                    parts = line.split(maxsplit=2)
                    if len(parts) >= 3:
                        key = parts[1].strip("'\"")
                        value = parts[2].strip("'\"")
                        self.config[current_section][key] = self._parse_value(value)
                
                elif line.startswith('list') and current_section:
                    # Parse list
                    parts = line.split(maxsplit=2)
                    if len(parts) >= 3:
                        key = parts[1].strip("'\"")
                        value = parts[2].strip("'\"")
                        if key not in self.config[current_section]:
                            self.config[current_section][key] = []
                        self.config[current_section][key].append(value)
        
        except Exception as e:
            print(f"Error parsing config: {e}")
            # Fallback to defaults
            self.config = DEFAULT_CONFIG.copy()
    
    def _parse_value(self, value: str):
        """Parse string value to appropriate type."""
        # Try boolean first (before integer)
        lower_val = value.lower()
        if lower_val == 'true' or value == '1':
            return True
        if lower_val == 'false' or value == '0':
            return False
        
        # Try integer
        try:
            return int(value)
        except ValueError:
            pass
        
        # Return as string
        return value
    
    def save_config(self):
        """Save configuration to file."""
        try:
            os.makedirs(os.path.dirname(self.config_path), exist_ok=True)
            with open(self.config_path, 'w') as f:
                for section, options in self.config.items():
                    f.write(f"config acctl-py '{section}'\n")
                    for key, value in options.items():
                        if isinstance(value, list):
                            for item in value:
                                f.write(f"\tlist {key} '{item}'\n")
                        else:
                            f.write(f"\toption {key} '{value}'\n")
                    f.write("\n")
        except Exception as e:
            print(f"Error saving config: {e}")
    
    def get_ac_config(self) -> Dict[str, Any]:
        """Get AC global configuration."""
        return self.config.get('global', {})
    
    def get_network_config(self) -> Dict[str, Any]:
        """Get network configuration."""
        return self.config.get('network', {})
    
    def get_resource_config(self) -> Dict[str, Any]:
        """Get resource configuration."""
        return self.config.get('resource', {})
    
    def get_security_config(self) -> Dict[str, Any]:
        """Get security configuration."""
        return self.config.get('security', {})
    
    def get_logging_config(self) -> Dict[str, Any]:
        """Get logging configuration."""
        return self.config.get('logging', {})
    
    def get_config(self, section: str = None) -> Dict[str, Any]:
        """Get entire config or specific section."""
        if section:
            return self.config.get(section, {})
        return self.config
    
    def set_option(self, section: str, key: str, value: Any):
        """Set a configuration option."""
        if section not in self.config:
            self.config[section] = {}
        self.config[section][key] = value
        self.save_config()
    
    def generate_uuid(self) -> str:
        """Generate a unique UUID for the AC."""
        import uuid
        return str(uuid.uuid4())


# Global config instance
config_manager = ConfigManager()
