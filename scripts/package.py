from pathlib import Path
from zipfile import ZIP_DEFLATED, ZipFile


ROOT = Path(__file__).resolve().parent.parent
ARCHIVE = ROOT / "submit.zip"
TEMPORARY_ARCHIVE = ROOT / "submit.tmp.zip"


def main():
    required_files = [ROOT / "Makefile", ROOT / "report.pdf"]
    for path in required_files:
        if not path.is_file():
            raise SystemExit(f"missing required file: {path.name}")

    source_directory = ROOT / "src"
    if not source_directory.is_dir():
        raise SystemExit("missing required directory: src")

    with ZipFile(TEMPORARY_ARCHIVE, "w", ZIP_DEFLATED) as archive:
        archive.writestr("Code/", "")
        archive.writestr("Code/src/", "")
        archive.write(ROOT / "Makefile", "Code/Makefile")

        for path in sorted(source_directory.rglob("*")):
            if path.is_file():
                archive.write(path, "Code/" + path.relative_to(ROOT).as_posix())

        archive.write(ROOT / "report.pdf", "report.pdf")

    TEMPORARY_ARCHIVE.replace(ARCHIVE)
    print(f"created {ARCHIVE.name}")


if __name__ == "__main__":
    main()
