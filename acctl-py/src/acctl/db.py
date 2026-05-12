#!/usr/bin/python3
"""
AC Controller Database Module
Provides SQLite-based persistent storage for AP information and configurations.
"""

import sqlite3
import os
import json
from typing import Dict, List, Optional, Any
from datetime import datetime

DB_PATH = '/etc/acctl-py/ac.db'


class DatabaseManager:
    """Manages SQLite database operations for AC Controller."""
    
    def __init__(self, db_path: str = DB_PATH):
        self.db_path = db_path
        self._init_db()
    
    def _init_db(self):
        """Initialize database tables."""
        os.makedirs(os.path.dirname(self.db_path), exist_ok=True)
        
        with sqlite3.connect(self.db_path) as conn:
            cursor = conn.cursor()
            
            # AP information table
            cursor.execute('''
                CREATE TABLE IF NOT EXISTS ap_info (
                    mac TEXT PRIMARY KEY,
                    name TEXT DEFAULT '',
                    model TEXT DEFAULT '',
                    firmware_version TEXT DEFAULT '',
                    ip_address TEXT DEFAULT '',
                    status TEXT DEFAULT 'offline',
                    uptime INTEGER DEFAULT 0,
                    clients INTEGER DEFAULT 0,
                    last_seen TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                    registered_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
                )
            ''')
            
            # AP configuration table
            cursor.execute('''
                CREATE TABLE IF NOT EXISTS ap_config (
                    mac TEXT PRIMARY KEY,
                    config_json TEXT DEFAULT '{}',
                    last_updated TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                    FOREIGN KEY (mac) REFERENCES ap_info(mac) ON DELETE CASCADE
                )
            ''')
            
            # AC configuration table
            cursor.execute('''
                CREATE TABLE IF NOT EXISTS ac_config (
                    key TEXT PRIMARY KEY,
                    value TEXT,
                    last_updated TIMESTAMP DEFAULT CURRENT_TIMESTAMP
                )
            ''')
            
            # Logs table
            cursor.execute('''
                CREATE TABLE IF NOT EXISTS logs (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    level TEXT DEFAULT 'info',
                    message TEXT,
                    module TEXT DEFAULT '',
                    timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP
                )
            ''')
            
            conn.commit()
    
    def add_ap(self, mac: str, info: Dict[str, Any]):
        """Add or update AP information."""
        with sqlite3.connect(self.db_path) as conn:
            cursor = conn.cursor()
            
            cursor.execute('''
                INSERT OR REPLACE INTO ap_info
                (mac, name, model, firmware_version, ip_address, status, uptime, clients, last_seen)
                VALUES (?, ?, ?, ?, ?, ?, ?, ?, CURRENT_TIMESTAMP)
            ''', (
                mac,
                info.get('name', ''),
                info.get('model', ''),
                info.get('firmware_version', ''),
                info.get('ip_address', ''),
                info.get('status', 'offline'),
                info.get('uptime', 0),
                info.get('clients', 0)
            ))
            
            conn.commit()
    
    def remove_ap(self, mac: str):
        """Remove AP from database."""
        with sqlite3.connect(self.db_path) as conn:
            cursor = conn.cursor()
            cursor.execute('DELETE FROM ap_info WHERE mac = ?', (mac,))
            conn.commit()
    
    def get_ap(self, mac: str) -> Optional[Dict[str, Any]]:
        """Get AP information by MAC address."""
        with sqlite3.connect(self.db_path) as conn:
            conn.row_factory = sqlite3.Row
            cursor = conn.cursor()
            cursor.execute('SELECT * FROM ap_info WHERE mac = ?', (mac,))
            row = cursor.fetchone()
            
            if row:
                return dict(row)
            return None
    
    def get_all_aps(self) -> List[Dict[str, Any]]:
        """Get all registered APs."""
        with sqlite3.connect(self.db_path) as conn:
            conn.row_factory = sqlite3.Row
            cursor = conn.cursor()
            cursor.execute('SELECT * FROM ap_info ORDER BY registered_at DESC')
            rows = cursor.fetchall()
            
            return [dict(row) for row in rows]
    
    def update_ap_status(self, mac: str, status: str):
        """Update AP status."""
        with sqlite3.connect(self.db_path) as conn:
            cursor = conn.cursor()
            cursor.execute(
                'UPDATE ap_info SET status = ?, last_seen = CURRENT_TIMESTAMP WHERE mac = ?',
                (status, mac)
            )
            conn.commit()
    
    def update_ap_stats(self, mac: str, uptime: int, clients: int):
        """Update AP statistics."""
        with sqlite3.connect(self.db_path) as conn:
            cursor = conn.cursor()
            cursor.execute(
                'UPDATE ap_info SET uptime = ?, clients = ?, last_seen = CURRENT_TIMESTAMP WHERE mac = ?',
                (uptime, clients, mac)
            )
            conn.commit()
    
    def save_ap_config(self, mac: str, config: Dict[str, Any]):
        """Save AP configuration."""
        config_json = json.dumps(config)
        with sqlite3.connect(self.db_path) as conn:
            cursor = conn.cursor()
            cursor.execute('''
                INSERT OR REPLACE INTO ap_config
                (mac, config_json, last_updated)
                VALUES (?, ?, CURRENT_TIMESTAMP)
            ''', (mac, config_json))
            conn.commit()
    
    def get_ap_config(self, mac: str) -> Dict[str, Any]:
        """Get AP configuration."""
        with sqlite3.connect(self.db_path) as conn:
            conn.row_factory = sqlite3.Row
            cursor = conn.cursor()
            cursor.execute('SELECT config_json FROM ap_config WHERE mac = ?', (mac,))
            row = cursor.fetchone()
            
            if row:
                return json.loads(row['config_json'])
            return {}
    
    def set_ac_config(self, key: str, value: Any):
        """Set AC configuration key-value pair."""
        value_str = json.dumps(value) if isinstance(value, (dict, list)) else str(value)
        with sqlite3.connect(self.db_path) as conn:
            cursor = conn.cursor()
            cursor.execute('''
                INSERT OR REPLACE INTO ac_config
                (key, value, last_updated)
                VALUES (?, ?, CURRENT_TIMESTAMP)
            ''', (key, value_str))
            conn.commit()
    
    def get_ac_config(self, key: str) -> Optional[Any]:
        """Get AC configuration value."""
        with sqlite3.connect(self.db_path) as conn:
            conn.row_factory = sqlite3.Row
            cursor = conn.cursor()
            cursor.execute('SELECT value FROM ac_config WHERE key = ?', (key,))
            row = cursor.fetchone()
            
            if row:
                try:
                    return json.loads(row['value'])
                except json.JSONDecodeError:
                    return row['value']
            return None
    
    def add_log(self, level: str, message: str, module: str = ''):
        """Add log entry."""
        with sqlite3.connect(self.db_path) as conn:
            cursor = conn.cursor()
            cursor.execute(
                'INSERT INTO logs (level, message, module) VALUES (?, ?, ?)',
                (level, message, module)
            )
            conn.commit()
    
    def get_logs(self, limit: int = 100) -> List[Dict[str, Any]]:
        """Get recent log entries."""
        with sqlite3.connect(self.db_path) as conn:
            conn.row_factory = sqlite3.Row
            cursor = conn.cursor()
            cursor.execute(
                'SELECT * FROM logs ORDER BY timestamp DESC LIMIT ?',
                (limit,)
            )
            rows = cursor.fetchall()
            
            return [dict(row) for row in rows]


# Global database instance
db_manager = DatabaseManager()
