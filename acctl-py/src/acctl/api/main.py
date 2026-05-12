#!/usr/bin/python3
"""
AC Controller API Server - Compatible with Python 3.6+
Uses standard library http.server instead of FastAPI
"""

import json
import threading
from http.server import HTTPServer, BaseHTTPRequestHandler
from urllib.parse import urlparse, parse_qs
import sys
import os

# Add parent directory to path for imports
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from acctl import ap_manager, config_manager, security_manager


class APIRequestHandler(BaseHTTPRequestHandler):
    """HTTP request handler for AC Controller API."""
    
    def _send_response(self, status_code, data=None, message='', success=True):
        """Send JSON response."""
        response = {
            'success': success,
            'message': message
        }
        if data is not None:
            response['data'] = data
        
        self.send_response(status_code)
        self.send_header('Content-Type', 'application/json')
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Access-Control-Allow-Methods', 'GET, POST, PUT, DELETE, OPTIONS')
        self.send_header('Access-Control-Allow-Headers', 'Content-Type, Authorization')
        self.end_headers()
        
        try:
            self.wfile.write(json.dumps(response).encode('utf-8'))
        except Exception as e:
            print(f"Error sending response: {e}")
    
    def _get_body(self):
        """Parse request body as JSON."""
        try:
            content_length = int(self.headers.get('Content-Length', 0))
            if content_length > 0:
                body = self.rfile.read(content_length).decode('utf-8')
                return json.loads(body)
            return {}
        except Exception:
            return {}
    
    def _authenticate(self):
        """Authenticate request using token."""
        auth_header = self.headers.get('Authorization', '')
        token = auth_header.replace('Bearer ', '').strip()
        
        if not token:
            token = self.headers.get('token', '')
        
        if not token:
            return False
        
        return security_manager.validate_token(token)
    
    def do_OPTIONS(self):
        """Handle OPTIONS request for CORS."""
        self.send_response(200)
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Access-Control-Allow-Methods', 'GET, POST, PUT, DELETE, OPTIONS')
        self.send_header('Access-Control-Allow-Headers', 'Content-Type, Authorization')
        self.end_headers()
    
    def do_GET(self):
        """Handle GET requests."""
        parsed = urlparse(self.path)
        path = parsed.path
        query = parse_qs(parsed.query)
        
        try:
            if path == '/api/v1/health':
                self._send_response(200, data={'status': 'healthy'}, message='AC Controller is running')
            
            elif path == '/api/v1/aps':
                if not self._authenticate():
                    self._send_response(401, message='Unauthorized', success=False)
                    return
                aps = [ap.to_dict() for ap in ap_manager.get_all_aps()]
                self._send_response(200, data=aps, message='AP list retrieved')
            
            elif path.startswith('/api/v1/aps/'):
                if not self._authenticate():
                    self._send_response(401, message='Unauthorized', success=False)
                    return
                mac = path.split('/')[-1]
                ap = ap_manager.get_ap_status(mac)
                if ap:
                    self._send_response(200, data=ap.to_dict(), message='AP found')
                else:
                    self._send_response(404, message='AP not found', success=False)
            
            elif path == '/api/v1/stats':
                if not self._authenticate():
                    self._send_response(401, message='Unauthorized', success=False)
                    return
                stats = ap_manager.get_stats()
                self._send_response(200, data=stats, message='Stats retrieved')
            
            elif path == '/api/v1/config':
                if not self._authenticate():
                    self._send_response(401, message='Unauthorized', success=False)
                    return
                config = config_manager.get_config()
                self._send_response(200, data=config, message='Config retrieved')
            
            elif path == '/docs':
                docs = self._generate_docs()
                self.send_response(200)
                self.send_header('Content-Type', 'text/html')
                self.end_headers()
                self.wfile.write(docs.encode('utf-8'))
            
            else:
                self._send_response(404, message='Endpoint not found', success=False)
        
        except Exception as e:
            self._send_response(500, message=f'Server error: {str(e)}', success=False)
    
    def do_POST(self):
        """Handle POST requests."""
        parsed = urlparse(self.path)
        path = parsed.path
        
        try:
            body = self._get_body()
            
            if path == '/api/v1/token':
                username = body.get('username', 'admin')
                # Simple token generation for compatibility
                token = security_manager.generate_token(username)
                self._send_response(200, data={
                    'token': token,
                    'expires_in': 86400,
                    'username': username
                }, message='Token generated successfully')
            
            elif path == '/api/v1/aps':
                if not self._authenticate():
                    self._send_response(401, message='Unauthorized', success=False)
                    return
                mac = body.get('mac')
                if not mac:
                    self._send_response(400, message='MAC address is required', success=False)
                    return
                
                ap_info = body.get('info', {})
                result = ap_manager.register_ap(mac, ap_info)
                if result:
                    self._send_response(201, data={'mac': mac}, message='AP registered successfully')
                else:
                    self._send_response(400, message='Failed to register AP', success=False)
            
            else:
                self._send_response(404, message='Endpoint not found', success=False)
        
        except Exception as e:
            self._send_response(500, message=f'Server error: {str(e)}', success=False)
    
    def do_PUT(self):
        """Handle PUT requests."""
        parsed = urlparse(self.path)
        path = parsed.path
        
        try:
            body = self._get_body()
            
            if path.startswith('/api/v1/aps/'):
                if not self._authenticate():
                    self._send_response(401, message='Unauthorized', success=False)
                    return
                mac = path.split('/')[-1]
                config = body.get('config', {})
                result = ap_manager.update_ap_config(mac, config)
                if result:
                    self._send_response(200, message='AP configuration updated')
                else:
                    self._send_response(404, message='AP not found', success=False)
            
            elif path == '/api/v1/config':
                if not self._authenticate():
                    self._send_response(401, message='Unauthorized', success=False)
                    return
                for section, options in body.items():
                    if isinstance(options, dict):
                        for key, value in options.items():
                            config_manager.set_option(section, key, value)
                self._send_response(200, message='Configuration updated')
            
            else:
                self._send_response(404, message='Endpoint not found', success=False)
        
        except Exception as e:
            self._send_response(500, message=f'Server error: {str(e)}', success=False)
    
    def do_DELETE(self):
        """Handle DELETE requests."""
        parsed = urlparse(self.path)
        path = parsed.path
        
        try:
            if path.startswith('/api/v1/aps/'):
                if not self._authenticate():
                    self._send_response(401, message='Unauthorized', success=False)
                    return
                mac = path.split('/')[-1]
                result = ap_manager.deregister_ap(mac)
                if result:
                    self._send_response(200, message='AP deregistered')
                else:
                    self._send_response(404, message='AP not found', success=False)
            
            else:
                self._send_response(404, message='Endpoint not found', success=False)
        
        except Exception as e:
            self._send_response(500, message=f'Server error: {str(e)}', success=False)
    
    def _generate_docs(self):
        """Generate simple HTML documentation."""
        return """
<!DOCTYPE html>
<html>
<head>
    <title>AC Controller API Documentation</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; }
        .endpoint { border: 1px solid #ccc; padding: 10px; margin: 10px; }
        .method { font-weight: bold; color: blue; }
        .path { font-family: monospace; color: green; }
    </style>
</head>
<body>
    <h1>AC Controller API</h1>
    
    <div class="endpoint">
        <span class="method">GET</span> <span class="path">/api/v1/health</span>
        <p>Health check endpoint (no authentication required)</p>
    </div>
    
    <div class="endpoint">
        <span class="method">POST</span> <span class="path">/api/v1/token</span>
        <p>Get API token. Body: {"username": "admin"}</p>
    </div>
    
    <div class="endpoint">
        <span class="method">GET</span> <span class="path">/api/v1/aps</span>
        <p>Get all APs (requires token)</p>
    </div>
    
    <div class="endpoint">
        <span class="method">GET</span> <span class="path">/api/v1/aps/{mac}</span>
        <p>Get AP by MAC (requires token)</p>
    </div>
    
    <div class="endpoint">
        <span class="method">POST</span> <span class="path">/api/v1/aps</span>
        <p>Register new AP (requires token). Body: {"mac": "...", "info": {...}}</p>
    </div>
    
    <div class="endpoint">
        <span class="method">PUT</span> <span class="path">/api/v1/aps/{mac}</span>
        <p>Update AP config (requires token). Body: {"config": {...}}</p>
    </div>
    
    <div class="endpoint">
        <span class="method">DELETE</span> <span class="path">/api/v1/aps/{mac}</span>
        <p>Deregister AP (requires token)</p>
    </div>
    
    <div class="endpoint">
        <span class="method">GET</span> <span class="path">/api/v1/stats</span>
        <p>Get statistics (requires token)</p>
    </div>
    
    <div class="endpoint">
        <span class="method">GET</span> <span class="path">/api/v1/config</span>
        <p>Get configuration (requires token)</p>
    </div>
    
    <div class="endpoint">
        <span class="method">PUT</span> <span class="path">/api/v1/config</span>
        <p>Update configuration (requires token)</p>
    </div>
</body>
</html>
"""
    
    def log_message(self, format, *args):
        """Suppress default logging."""
        pass


def start_api_server(host='0.0.0.0', port=8080):
    """Start the API server."""
    server = HTTPServer((host, port), APIRequestHandler)
    print(f"API server listening on {host}:{port}")
    server.serve_forever()


if __name__ == '__main__':
    start_api_server()
