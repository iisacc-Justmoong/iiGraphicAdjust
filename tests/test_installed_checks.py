"""The installed checks must preserve existing manifests and test builds."""
from pathlib import Path
import unittest
import installed_checks as checks
import ii_file_provider as provider

class InstalledChecksPolicy(unittest.TestCase):
    def test_manifest_redirect_requires_exact_paths(self):
        build, workspace = Path("/project/build"), Path("/project/resources/.t/abcd")
        text = str(build / "install_local_manifest.txt") + "\n" + str(build / "${CMAKE_INSTALL_MANIFEST}")
        self.assertNotIn(str(build), checks.redirect_manifest(text, build, workspace))
        with self.assertRaises(ValueError):
            checks.redirect_manifest(text + "\n" + text, build, workspace)

    def test_existing_consumer_build_is_not_overwritten(self):
        owner = Path.cwd() / "consumer-policy"
        provider.mkdir(owner, exist_ok=False)
        try:
            existing = checks.reserve_build(owner, "abcd")
            with self.assertRaises(FileExistsError): checks.reserve_build(owner, "abcd")
            with self.assertRaises(ValueError): checks.reserve_build(owner, "../escape")
            self.assertTrue(existing.is_dir())
            existing.rmdir()
        finally:
            owner.rmdir()

if __name__ == "__main__":
    unittest.main()
