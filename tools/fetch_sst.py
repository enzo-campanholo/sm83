"""Downloads the SingleStepTests files of the trial opcodes, pinned by commit
and checked by SHA-256.

    python tools/fetch_sst.py [DEST]     (default: tests/sst)

Existing files with the right checksum are kept. Standard library only.
"""

import hashlib
import sys
import urllib.request
from pathlib import Path

REPO = "SingleStepTests/sm83"
COMMIT = "f9c30210245dd691661db39f5ace022c465ecc2f"
URL = f"https://raw.githubusercontent.com/{REPO}/{COMMIT}/v1/{{name}}"

# One file per opcode implemented in the trial.
FILES = {
    "00.json": "fa7f656f78aec939dea1094fd48f555dbde9a796458c5ae2b3e5c236e9720d66",
    "04.json": "f382e57f629594b6c0c8b524e317792f6af4337d274f6a32c85cca1770bc21c1",
    "06.json": "eacd982507bd57c32f20d60ba56d0dfb1e1f63d665745eac81c11133b101fc98",
    "20.json": "720734dba36eba632bbea0d8eca39785f8c34bf871b15991d040e9230de9489f",
    "77.json": "30722488e77f281de2305fa3a63885ed640f8bed4413fafe50f66ee42a0abad7",
    "7e.json": "3f7f33396836174db6e9f9c51ae42dca86e32189d6f62bbfc170f6e8faf1fd37",
    "80.json": "51b3e835b0aa594056abc2f70da3a3e7c639ea65d897b46f06aab3476c0b8d53",
    "c3.json": "2a321e9d4c9f53a011ca3ab3d7d59799b924ac0e5405d048fd9487ee8270adcc",
    "c5.json": "146c01464453b65cb00142578e7a574fde81a668bb3bfbf34eddbd458baaf887",
    "c6.json": "9b5672e926f173e9e19f51a0dff7362e592b7db0f0c44c40c715a5609dccbc34",
    "c9.json": "fc2e7a93d34d7fa82a7a7940516a16c1bfb0c9415c8c22f174d6d183783661fd",
    "cd.json": "5ec93a5c07723eb462fd66fce945a6560c3f9a95088f8a3a4bebd5ddf80443da",
    "f1.json": "d8c7a9eb386cd3213011fbe2460acf39630c5e2aa690f53336b247065a523ee3",
}


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def main() -> int:
    dest = Path(sys.argv[1] if len(sys.argv) > 1 else "tests/sst")
    dest.mkdir(parents=True, exist_ok=True)
    failed = False
    for name, digest in FILES.items():
        path = dest / name
        if path.exists() and sha256(path.read_bytes()) == digest:
            continue
        with urllib.request.urlopen(URL.format(name=name), timeout=60) as resp:
            data = resp.read()
        if sha256(data) != digest:
            print(f"{name}: checksum mismatch", file=sys.stderr)
            failed = True
            continue
        path.write_bytes(data)
        print(f"{name}: downloaded")
    if not failed:
        print(f"{len(FILES)} files in {dest} (commit {COMMIT[:12]})")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
