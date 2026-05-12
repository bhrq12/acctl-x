#!/usr/bin/python3
"""
Unit Tests for Database Module
"""

import pytest
import os
import tempfile
import time

# Add src directory to path
import sys
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', '..', 'src'))

from acctl.db import DatabaseManager


class TestDatabaseManager:
    """Tests for DatabaseManager class."""
    
    def _cleanup_db(self, db_path):
        """Cleanup database file with retry for Windows file locking."""
        for _ in range(3):
            try:
                if os.path.exists(db_path):
                    os.unlink(db_path)
                break
            except PermissionError:
                time.sleep(0.1)
    
    def test_add_and_get_ap(self):
        """Test adding and retrieving AP."""
        with tempfile.NamedTemporaryFile(suffix='.db', delete=False) as f:
            db_path = f.name
        
        try:
            db = DatabaseManager(db_path)
            
            # Add AP
            ap_info = {
                'name': 'Test AP',
                'model': 'AP-123',
                'firmware_version': '1.0.0',
                'ip_address': '192.168.1.100',
                'status': 'online',
                'uptime': 3600,
                'clients': 10
            }
            db.add_ap('00:11:22:33:44:55', ap_info)
            
            # Retrieve AP
            retrieved = db.get_ap('00:11:22:33:44:55')
            assert retrieved is not None
            assert retrieved['name'] == 'Test AP'
            assert retrieved['model'] == 'AP-123'
            assert retrieved['status'] == 'online'
        finally:
            self._cleanup_db(db_path)
    
    def test_remove_ap(self):
        """Test removing AP."""
        with tempfile.NamedTemporaryFile(suffix='.db', delete=False) as f:
            db_path = f.name
        
        try:
            db = DatabaseManager(db_path)
            db.add_ap('00:11:22:33:44:55', {'name': 'Test'})
            
            assert db.get_ap('00:11:22:33:44:55') is not None
            
            db.remove_ap('00:11:22:33:44:55')
            assert db.get_ap('00:11:22:33:44:55') is None
        finally:
            self._cleanup_db(db_path)
    
    def test_get_all_aps(self):
        """Test getting all APs."""
        with tempfile.NamedTemporaryFile(suffix='.db', delete=False) as f:
            db_path = f.name
        
        try:
            db = DatabaseManager(db_path)
            
            db.add_ap('00:11:22:33:44:01', {'name': 'AP1'})
            db.add_ap('00:11:22:33:44:02', {'name': 'AP2'})
            
            aps = db.get_all_aps()
            assert len(aps) == 2
        finally:
            self._cleanup_db(db_path)
    
    def test_update_ap_status(self):
        """Test updating AP status."""
        with tempfile.NamedTemporaryFile(suffix='.db', delete=False) as f:
            db_path = f.name
        
        try:
            db = DatabaseManager(db_path)
            db.add_ap('00:11:22:33:44:55', {'name': 'Test', 'status': 'online'})
            
            db.update_ap_status('00:11:22:33:44:55', 'offline')
            
            ap = db.get_ap('00:11:22:33:44:55')
            assert ap['status'] == 'offline'
        finally:
            self._cleanup_db(db_path)
    
    def test_save_and_get_ap_config(self):
        """Test saving and retrieving AP configuration."""
        with tempfile.NamedTemporaryFile(suffix='.db', delete=False) as f:
            db_path = f.name
        
        try:
            db = DatabaseManager(db_path)
            config = {'ssid': 'TestWiFi', 'channel': 6, 'security': 'WPA2'}
            
            db.save_ap_config('00:11:22:33:44:55', config)
            
            retrieved = db.get_ap_config('00:11:22:33:44:55')
            assert retrieved == config
        finally:
            self._cleanup_db(db_path)


if __name__ == '__main__':
    pytest.main([__file__, '-v'])