#!/usr/bin/python3
"""
Unit Tests for AP Manager Module
"""

import pytest
import os
import tempfile

# Add src directory to path
import sys
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', '..', 'src'))

from acctl.ap_manager import APManager, APInfo


class TestAPManager:
    """Tests for APManager class."""
    
    def setup_method(self):
        """Setup method - create a fresh manager for each test."""
        self.manager = APManager()
        # Clear any pre-existing APs from previous tests
        with self.manager.lock:
            self.manager.aps.clear()
    
    def test_register_ap(self):
        """Test registering an AP."""
        self.manager.register_ap('00:11:22:33:44:55', {
            'name': 'Test AP',
            'model': 'AP-123',
            'status': 'online'
        })
        
        ap = self.manager.get_ap_status('00:11:22:33:44:55')
        assert ap is not None
        assert ap.mac == '00:11:22:33:44:55'
        assert ap.name == 'Test AP'
        assert ap.status == 'online'
    
    def test_deregister_ap(self):
        """Test deregistering an AP."""
        self.manager.register_ap('00:11:22:33:44:55', {'name': 'Test'})
        assert self.manager.get_ap_status('00:11:22:33:44:55') is not None
        
        result = self.manager.deregister_ap('00:11:22:33:44:55')
        assert result is True
        assert self.manager.get_ap_status('00:11:22:33:44:55') is None
    
    def test_deregister_nonexistent_ap(self):
        """Test deregistering non-existent AP."""
        result = self.manager.deregister_ap('00:11:22:33:44:55')
        assert result is False
    
    def test_get_all_aps(self):
        """Test getting all APs."""
        self.manager.register_ap('00:11:22:33:44:01', {'name': 'AP1'})
        self.manager.register_ap('00:11:22:33:44:02', {'name': 'AP2'})
        
        aps = self.manager.get_all_aps()
        assert len(aps) == 2
        
        macs = [ap.mac for ap in aps]
        assert '00:11:22:33:44:01' in macs
        assert '00:11:22:33:44:02' in macs
    
    def test_update_ap_config(self):
        """Test updating AP configuration."""
        self.manager.register_ap('00:11:22:33:44:55', {'name': 'Test'})
        
        config = {'ssid': 'WiFi', 'channel': 6}
        result = self.manager.update_ap_config('00:11:22:33:44:55', config)
        assert result is True
        
        retrieved = self.manager.get_ap_config('00:11:22:33:44:55')
        assert retrieved == config
    
    def test_update_nonexistent_ap_config(self):
        """Test updating non-existent AP configuration."""
        result = self.manager.update_ap_config('00:11:22:33:44:55', {'ssid': 'Test'})
        assert result is False


class TestAPInfo:
    """Tests for APInfo class."""
    
    def test_ap_info_creation(self):
        """Test APInfo object creation."""
        ap = APInfo(
            '00:11:22:33:44:55',
            name='Test AP',
            model='AP-123',
            status='online'
        )
        
        assert ap.mac == '00:11:22:33:44:55'
        assert ap.name == 'Test AP'
        assert ap.model == 'AP-123'
        assert ap.status == 'online'
    
    def test_ap_info_to_dict(self):
        """Test converting APInfo to dictionary."""
        ap = APInfo(
            '00:11:22:33:44:55',
            name='Test AP',
            status='online',
            clients=5
        )
        
        ap_dict = ap.to_dict()
        
        assert ap_dict['mac'] == '00:11:22:33:44:55'
        assert ap_dict['name'] == 'Test AP'
        assert ap_dict['status'] == 'online'
        assert ap_dict['clients'] == 5


if __name__ == '__main__':
    pytest.main([__file__, '-v'])