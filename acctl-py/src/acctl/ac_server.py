#!/usr/bin/python3
"""
AC Controller Server Main Entry Point
Compatible with Python 3.6+
"""

import os
import sys
import time
import signal
import threading
from multiprocessing import Process

# Add src directory to path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

from acctl.logger import logger
from acctl.config import config_manager
from acctl.ap_manager import ap_manager


def start_api_server():
    """Start API server using standard library http.server."""
    try:
        from acctl.api.main import start_api_server
        network_config = config_manager.get_network_config()
        listen_port = network_config.get('listen_port', 8080)
        
        start_api_server(host='0.0.0.0', port=listen_port)
    except Exception as e:
        logger.error(f"Failed to start API server: {e}")


def signal_handler(signum, frame):
    """Handle shutdown signals."""
    logger.info(f"Received signal {signum}, shutting down...")
    ap_manager.stop()
    os._exit(0)


def main():
    """Main entry point."""
    # Register signal handlers
    signal.signal(signal.SIGINT, signal_handler)
    signal.signal(signal.SIGTERM, signal_handler)
    
    # Load configuration
    config = config_manager.get_config()
    logger.info(f"AC Controller starting with config: {config}")
    
    # Generate UUID if auto
    global_config = config_manager.get_ac_config()
    if global_config.get('uuid') == 'auto':
        uuid = config_manager.generate_uuid()
        config_manager.set_option('global', 'uuid', uuid)
        logger.info(f"Generated AC UUID: {uuid}")
    
    # Start AP manager
    ap_manager.start()
    
    # Start API server in separate process
    api_process = Process(target=start_api_server)
    api_process.start()
    
    logger.info("AC Controller started successfully")
    
    # Main loop
    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        logger.info("Received keyboard interrupt")
    finally:
        ap_manager.stop()
        if api_process.is_alive():
            api_process.terminate()
            api_process.join()
        logger.info("AC Controller stopped")


if __name__ == "__main__":
    main()