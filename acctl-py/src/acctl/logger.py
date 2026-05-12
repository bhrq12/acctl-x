#!/usr/bin/python3
"""
AC Controller Logger Module
Provides unified logging functionality with multiple output destinations.
"""

import logging
import os
import sys
from logging.handlers import SysLogHandler


class Logger:
    """Centralized logging system for AC Controller."""
    
    def __init__(self, name='acctl', level='info', use_syslog=True):
        self.logger = logging.getLogger(name)
        self.logger.setLevel(self._get_log_level(level))
        self.logger.propagate = False
        
        # Clear existing handlers to avoid duplicates
        self.logger.handlers.clear()
        
        # Create formatters
        self.formatter = logging.Formatter(
            '%(asctime)s [%(levelname)s] [%(module)s] %(message)s',
            datefmt='%Y-%m-%d %H:%M:%S'
        )
        
        # File handler
        self._setup_file_handler()
        
        # Console handler
        self._setup_console_handler()
        
        # Syslog handler
        if use_syslog:
            self._setup_syslog_handler()
    
    def _get_log_level(self, level_str):
        """Convert string level to logging constant."""
        levels = {
            'debug': logging.DEBUG,
            'info': logging.INFO,
            'warn': logging.WARNING,
            'error': logging.ERROR,
            'critical': logging.CRITICAL
        }
        return levels.get(level_str.lower(), logging.INFO)
    
    def _setup_file_handler(self):
        """Setup file logging handler."""
        log_dir = '/var/log/acctl'
        os.makedirs(log_dir, exist_ok=True)
        log_file = os.path.join(log_dir, 'ac_server.log')
        
        file_handler = logging.FileHandler(log_file)
        file_handler.setLevel(logging.DEBUG)
        file_handler.setFormatter(self.formatter)
        self.logger.addHandler(file_handler)
    
    def _setup_console_handler(self):
        """Setup console logging handler."""
        console_handler = logging.StreamHandler(sys.stdout)
        console_handler.setLevel(logging.INFO)
        console_handler.setFormatter(self.formatter)
        self.logger.addHandler(console_handler)
    
    def _setup_syslog_handler(self):
        """Setup syslog handler for system logging."""
        try:
            syslog_handler = SysLogHandler(address='/dev/log')
            syslog_formatter = logging.Formatter(
                '%(name)s[%(process)d]: %(levelname)s %(message)s'
            )
            syslog_handler.setFormatter(syslog_formatter)
            self.logger.addHandler(syslog_handler)
        except Exception:
            # Syslog might not be available in some environments
            self.logger.debug("Syslog handler not available")
    
    def debug(self, msg, **kwargs):
        """Log debug message."""
        self.logger.debug(msg, **kwargs)
    
    def info(self, msg, **kwargs):
        """Log info message."""
        self.logger.info(msg, **kwargs)
    
    def warn(self, msg, **kwargs):
        """Log warning message."""
        self.logger.warning(msg, **kwargs)
    
    def error(self, msg, **kwargs):
        """Log error message."""
        self.logger.error(msg, **kwargs)
    
    def critical(self, msg, **kwargs):
        """Log critical message."""
        self.logger.critical(msg, **kwargs)


# Global logger instance
logger = Logger()
