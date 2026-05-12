#!/usr/bin/python3
"""
Unit Tests for Configuration Management
"""

import pytest
import os
import tempfile

# Add src directory to path
import sys
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', '..', 'src'))

from acctl.config import ConfigManager, DEFAULT_CONFIG


class TestConfigManager:
    """Tests for ConfigManager class."""
    
    def test_default_config(self):
        """Test loading default configuration."""
        with tempfile.NamedTemporaryFile(mode='w', suffix='.config', delete=False) as f:
            temp_path = f.name
        
        try:
            config = ConfigManager(temp_path)
            assert config.get_config() == DEFAULT_CONFIG
        finally:
            os.unlink(temp_path)
    
    def test_set_option(self):
        """Test setting configuration option."""
        with tempfile.NamedTemporaryFile(mode='w', suffix='.config', delete=False) as f:
            temp_path = f.name
        
        try:
            config = ConfigManager(temp_path)
            config.set_option('global', 'name', 'Test-AC')
            
            loaded = ConfigManager(temp_path)
            assert loaded.get_ac_config()['name'] == 'Test-AC'
        finally:
            os.unlink(temp_path)
    
    def test_generate_uuid(self):
        """Test UUID generation."""
        config = ConfigManager('/tmp/test-config')
        uuid = config.generate_uuid()
        
        assert len(uuid) == 36  # Standard UUID length
        assert uuid.count('-') == 4  # UUID format check
    
    def test_parse_bool(self):
        """Test boolean value parsing."""
        with tempfile.NamedTemporaryFile(mode='w', suffix='.config', delete=False) as f:
            f.write("""
config acctl-py 'test'
    option enabled '1'
    option debug 'true'
    option disabled '0'
""")
            temp_path = f.name
        
        try:
            config = ConfigManager(temp_path)
            test_config = config.get_config('test')
            
            assert test_config['enabled'] is True
            assert test_config['debug'] is True
            assert test_config['disabled'] is False
        finally:
            os.unlink(temp_path)


if __name__ == '__main__':
    pytest.main([__file__, '-v'])
