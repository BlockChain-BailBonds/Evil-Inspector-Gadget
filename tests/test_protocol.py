import unittest
from protocol.eig_protocol import make_request, verify, encode_frame

class ProtocolTests(unittest.TestCase):
    def test_round_trip(self):
        req = make_request("ping", "1", b"test-secret", nonce="abc", timestamp=100)
        verify(req, b"test-secret", now=100)
        self.assertTrue(encode_frame(req).endswith(b"\n"))

    def test_tamper_rejected(self):
        req = make_request("ping", "1", b"test-secret", nonce="abc", timestamp=100)
        req["op"] = "stop"
        with self.assertRaises(ValueError):
            verify(req, b"test-secret", now=100)

