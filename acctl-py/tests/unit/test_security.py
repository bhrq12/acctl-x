#!/usr/bin/python3
"""
Unit Tests for Security Module
"""

import pytest
import os

# Add src directory to path
import sys
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', '..', 'src'))

from acctl.security import SecurityManager


class TestSecurityManager:
    """Tests for SecurityManager class."""
    
    def test_generate_token(self):
        """Test token generation."""
        security = SecurityManager()
        token = security.generate_token('testuser')
        
        assert len(token) == 32
        assert security.token_count == 1
    
    def test_validate_token(self):
        """Test token validation."""
        security = SecurityManager()
        token = security.generate_token('testuser')
        
        assert security.validate_token(token) is True
        assert security.validate_token('invalid-token') is False
    
    def test_token_expiration(self):
        """Test token expiration."""
        security = SecurityManager()
        security.tokens = {}
        security.token_count = 0
        
        # Create a token that's already expired
        import time
        expired_token = 'expired123456789012345678901234567890'
        security.tokens[expired_token] = {
            'username': 'test',
            'created': time.time() - 86401,  # 1 second past expiration
            'expires': time.time() - 1
        }
        security.token_count = 1
        
        assert security.validate_token(expired_token) is False
        assert security.token_count == 0
    
    def test_chap_authentication(self):
        """Test CHAP authentication."""
        security = SecurityManager()
        challenge = 'test_challenge'
        password = 'secret123'
        
        # Calculate expected response manually
        import hashlib
        expected_response = hashlib.md5((challenge + password).encode()).hexdigest()
        
        # Test valid authentication
        assert security.chap_authenticate(challenge, expected_response, password) is True
        
        # Test invalid response
        assert security.chap_authenticate(challenge, 'wrong_response', password) is False
    
    def test_hash_password(self):
        """Test password hashing."""
        security = SecurityManager()
        password = 'test_password'
        hashed = security.hash_password(password)
        
        assert len(hashed) == 64  # SHA-256 produces 64 hex characters
        assert hashed == security.hash_password(password)  # Consistent hashing


if __name__ == '__main__':
    pytest.main([__file__, '-v'])
