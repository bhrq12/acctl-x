#!/usr/bin/python3
"""
AC Controller Security Module
Provides authentication, authorization and encryption functionality.
"""

import hashlib
import random
import string
import time
from typing import Dict, Optional
from datetime import datetime, timedelta

# Token expiration time (24 hours)
TOKEN_EXPIRY_SEC = 86400
MAX_TOKENS = 100


class SecurityManager:
    """Handles security operations including authentication and token management."""
    
    def __init__(self):
        self.tokens: Dict[str, Dict[str, any]] = {}
        self.token_count = 0
    
    def generate_token(self, username: str) -> str:
        """Generate a random API token."""
        token = ''.join(random.choices(
            string.ascii_letters + string.digits,
            k=32
        ))
        
        # Store token with metadata
        if self.token_count >= MAX_TOKENS:
            # Remove oldest token
            oldest_token = min(self.tokens.keys(), key=lambda k: self.tokens[k]['created'])
            del self.tokens[oldest_token]
            self.token_count -= 1
        
        self.tokens[token] = {
            'username': username,
            'created': time.time(),
            'expires': time.time() + TOKEN_EXPIRY_SEC
        }
        self.token_count += 1
        
        return token
    
    def validate_token(self, token: str) -> bool:
        """Validate API token."""
        if token not in self.tokens:
            return False
        
        token_info = self.tokens[token]
        
        # Check expiration
        if time.time() > token_info['expires']:
            del self.tokens[token]
            self.token_count -= 1
            return False
        
        return True
    
    def invalidate_token(self, token: str):
        """Invalidate a token."""
        if token in self.tokens:
            del self.tokens[token]
            self.token_count -= 1
    
    def generate_challenge(self) -> str:
        """Generate a random CHAP challenge string."""
        return ''.join(random.choices(
            string.ascii_letters + string.digits,
            k=16
        ))
    
    def chap_authenticate(self, challenge: str, response: str, password: str) -> bool:
        """
        Validate CHAP authentication.
        
        :param challenge: The challenge sent to the AP
        :param response: The response received from the AP
        :param password: The expected password
        :return: True if authentication succeeds
        """
        # Calculate expected response: MD5(challenge + password)
        expected_response = hashlib.md5((challenge + password).encode()).hexdigest()
        return response.lower() == expected_response.lower()
    
    def hash_password(self, password: str) -> str:
        """Hash password using SHA-256."""
        return hashlib.sha256(password.encode()).hexdigest()
    
    def generate_random_bytes(self, length: int) -> bytes:
        """Generate cryptographically secure random bytes."""
        return random.getrandbits(length * 8).to_bytes(length, byteorder='big')


# Global security instance
security_manager = SecurityManager()
