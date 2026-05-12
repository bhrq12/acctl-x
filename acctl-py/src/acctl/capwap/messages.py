#!/usr/bin/python3
"""
CAPWAP Protocol Constants and Message Definitions
"""

# CAPWAP message types
CAPWAP_MESSAGE_TYPES = {
    1: 'DISCOVER_REQUEST',
    2: 'DISCOVER_RESPONSE',
    3: 'JOIN_REQUEST',
    4: 'JOIN_RESPONSE',
    5: 'CONFIGURATION_REQUEST',
    6: 'CONFIGURATION_RESPONSE',
    7: 'CAPWAP_CONTROL_MESSAGE',
    8: 'CHANGE_STATE_EVENT_REQUEST',
    9: 'CHANGE_STATE_EVENT_RESPONSE',
    10: 'CONFIGURATION_STATUS_REQUEST',
    11: 'CONFIGURATION_STATUS_RESPONSE',
    12: 'ECHO_REQUEST',
    13: 'ECHO_RESPONSE',
    14: 'IMAGE_DATA_REQUEST',
    15: 'IMAGE_DATA_RESPONSE',
    16: 'REBOOT_REQUEST',
    17: 'REBOOT_RESPONSE',
    18: 'DECRYPTION_KEY_REQUEST',
    19: 'DECRYPTION_KEY_RESPONSE',
    20: 'CONTROL_HEADER_MESSAGE'
}

# CAPWAP header fields
CAPWAP_HEADER_LENGTH = 12

# CAPWAP element types
CAPWAP_ELEMENT_TYPES = {
    1: 'AC_DESCRIPTOR',
    2: 'AC_NAME',
    3: 'AC_COOKIE',
    4: 'AP_DESCRIPTOR',
    5: 'AP_NAME',
    6: 'AP_MAC_TYPE',
    7: 'AP_MAC',
    8: 'AP_RADIO_ID',
    9: 'AP_SESSION_ID',
    10: 'AUTHENTICATION_TYPE',
    11: 'CONTROL_IPV4_ADDRESS',
    12: 'CONTROL_IPV6_ADDRESS',
    13: 'CONTROL_PORT',
    14: 'DISCOVERY_TYPE',
    15: 'DTLS_CIPHER_SUITES',
    16: 'DTLS_PSK',
    17: 'DTLS_RANDOM',
    18: 'DTLS_SERVER_CERTIFICATE',
    19: 'ECHO_DATA',
    20: 'IEEE80211_WTP_RADIO_INFORMATION',
    21: 'IMAGE_DATA',
    22: 'IMAGE_ID',
    23: 'IMAGE_INFORMATION',
    24: 'IPV4_ADDRESS',
    25: 'IPV6_ADDRESS',
    26: 'LOCAL_IPV4_ADDRESS',
    27: 'LOCAL_IPV6_ADDRESS',
    28: 'MAX_MESSAGE_LENGTH',
    29: 'RADIO_ADMIN_STATE',
    30: 'RADIO_OPERATIONAL_STATE',
    31: 'REBOOT_TYPE',
    32: 'SESSION_TIMEOUT',
    33: 'STATUS_CODE',
    34: 'TIMESTAMP',
    35: 'TRANSPORT_TYPE',
    36: 'UNKNOWN_ELEMENT',
    37: 'VENDOR_SPECIFIC',
    38: 'WTP_BOOTSTRAP',
    39: 'WTP_FRAME_TUNNEL_MODE',
    40: 'WTP_MAC',
    41: 'WTP_NAME',
    42: 'WTP_RADIO_INFORMATION',
    43: 'WTP_RESOURCE',
    44: 'WTP_SECURITY_CAPABILITY'
}

# Status codes
CAPWAP_STATUS_CODES = {
    0: 'Success',
    1: 'Invalid frame',
    2: 'Invalid message type',
    3: 'Invalid message length',
    4: 'Invalid element',
    5: 'Invalid element length',
    6: 'Missing mandatory element',
    7: 'Invalid value',
    8: 'AC full',
    9: 'Denied',
    10: 'Timeout',
    11: 'Unsupported version',
    12: 'Invalid session',
    13: 'Certificate error',
    14: 'Decryption error',
    15: 'Authentication failed',
    16: 'DTLS required',
    17: 'DTLS failure'
}

# Default CAPWAP ports
CAPWAP_CONTROL_PORT = 5246
CAPWAP_DATA_PORT = 5247

# CAPWAP version
CAPWAP_VERSION = 0x01

# Discovery types
DISCOVERY_TYPE_UNICAST = 0
DISCOVERY_TYPE_BROADCAST = 1
DISCOVERY_TYPE_MULTICAST = 2
