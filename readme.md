# Python package to read Blackmagic design RAW .braw files metadata

## Repository structure

- `/src/` - contains the c++ source to build the shared object
- `/python_package/` - contains the python package plus a demo cli
- `/python_package/braw_metadata/` - the actual package for importing into python scripts

## Usage

### Python API

```python
from braw_metadata import bmd_metadata
# extract metadata fields as dictionary
metadata = bmd_metadata.read_metadata(file_path)
```

### Demo cli (macos 13)

- `/python_package/dist/cli_macos` - cli binary location

- invoke cli with the following options:

```
    -i, --inputpath TEXT            Path to the directory containing the files to be processed
    -v, --verbose                   Output metadata to terminal
    -o, --outputpath                Path to the output file. If not specified, use working directory
    --help                          Show this message and exit.
```

