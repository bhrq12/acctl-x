#!/usr/bin/python3
"""
DTLS Encryption Module for CAPWAP
Provides DTLS encryption support for CAPWAP control channel.
"""

import ssl
import socket
from typing import Optional

try:
    from cryptography import x509
    from cryptography.x509.oid import NameOID
    from cryptography.hazmat.primitives import serialization, hashes
    from cryptography.hazmat.primitives.asymmetric import rsa
    from cryptography.hazmat.backends import default_backend
    HAS_CRYPTOGRAPHY = True
except ImportError:
    HAS_CRYPTOGRAPHY = False


class DTLSServer:
    """DTLS server implementation for CAPWAP."""
    
    def __init__(self):
        self.context = None
        self.socket = None
        self.enabled = HAS_CRYPTOGRAPHY
    
    def init_context(self, cert_path: str = None, key_path: str = None):
        """Initialize DTLS context."""
        if not HAS_CRYPTOGRAPHY:
            return False
        
        try:
            self.context = ssl.SSLContext(ssl.PROTOCOL_DTLS_SERVER)
            
            if cert_path and key_path:
                # Use existing certificate
                self.context.load_cert_chain(certfile=cert_path, keyfile=key_path)
            else:
                # Generate self-signed certificate
                self._generate_self_signed_cert()
            
            self.context.set_ciphers('ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES128-GCM-SHA256')
            return True
        except Exception as e:
            print(f"Error initializing DTLS context: {e}")
            return False
    
    def _generate_self_signed_cert(self):
        """Generate self-signed certificate for DTLS."""
        # Generate private key
        private_key = rsa.generate_private_key(
            public_exponent=65537,
            key_size=2048,
            backend=default_backend()
        )
        
        # Generate certificate
        subject = issuer = x509.Name([
            x509.NameAttribute(NameOID.COUNTRY_NAME, "CN"),
            x509.NameAttribute(NameOID.STATE_OR_PROVINCE_NAME, "Beijing"),
            x509.NameAttribute(NameOID.LOCALITY_NAME, "Beijing"),
            x509.NameAttribute(NameOID.ORGANIZATION_NAME, "AC Controller"),
            x509.NameAttribute(NameOID.COMMON_NAME, "acctl-py"),
        ])
        
        cert = x509.CertificateBuilder().subject_name(
            subject
        ).issuer_name(
            issuer
        ).public_key(
            private_key.public_key()
        ).serial_number(
            x509.random_serial_number()
        ).not_valid_before(
            x509.datetime.datetime.utcnow()
        ).not_valid_after(
            x509.datetime.datetime.utcnow() + x509.timedelta(days=365)
        ).add_extension(
            x509.SubjectAlternativeName([x509.DNSName("localhost")]),
            critical=False,
        ).sign(private_key, hashes.SHA256(), default_backend())
        
        # Store in memory
        self._private_key = private_key
        self._certificate = cert
        
        # Load into context
        self.context.load_cert_chain(
            certfile=None,
            keyfile=None,
            password=None,
            cert_data=cert.public_bytes(serialization.Encoding.PEM),
            key_data=private_key.private_bytes(
                encoding=serialization.Encoding.PEM,
                format=serialization.PrivateFormat.TraditionalOpenSSL,
                encryption_algorithm=serialization.NoEncryption(),
            )
        )
    
    def wrap_socket(self, sock: socket.socket) -> Optional[ssl.SSLSocket]:
        """Wrap existing socket with DTLS."""
        if not self.context:
            return None
        
        try:
            return self.context.wrap_socket(sock, server_side=True)
        except Exception as e:
            print(f"Error wrapping socket: {e}")
            return None
    
    def encrypt(self, data: bytes) -> bytes:
        """Encrypt data (placeholder)."""
        return data
    
    def decrypt(self, data: bytes) -> bytes:
        """Decrypt data (placeholder)."""
        return data
