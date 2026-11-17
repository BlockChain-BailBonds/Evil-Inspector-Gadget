import tempfile
import unittest
from pathlib import Path

from lab.workflow import analyze_capture, extract_identifiers, load_profile, simulation


class LabWorkflowTests(unittest.TestCase):
    def test_extracts_optional_visible_identifiers(self):
        result = extract_identifiers("Brand Flipper Model FZ-1 FCC ID ABC-123", ["front.jpg"])
        self.assertEqual(result["fcc_ids"], ["ABC-123"])
        self.assertEqual(result["models"], ["FZ-1"])
        self.assertFalse(result["ambiguous"])

    def test_ambiguous_intake_is_not_guessed(self):
        self.assertTrue(extract_identifiers("black handheld device")["ambiguous"])

    def test_profile_is_read_only(self):
        profile = load_profile()["profile"]
        self.assertFalse(profile["transmission_enabled"])
        self.assertIn("GPIO control", profile["unsupported"])

    def test_simulation_is_unverified(self):
        result = simulation(load_profile())
        self.assertTrue(result["simulated"])
        self.assertEqual(result["hardware_test"], "UNVERIFIED")

    def test_capture_analysis_is_local_only(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "capture.bin"
            path.write_bytes(b"synthetic")
            result = analyze_capture(path)
            self.assertFalse(result["transmitted"])
            self.assertEqual(result["bytes"], 9)


if __name__ == "__main__":
    unittest.main()
