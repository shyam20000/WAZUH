import json
import platform
import subprocess
from pathlib import Path

import pytest
from jsonschema import validate
from jsonschema.exceptions import ValidationError


def call_binary(binary_path, parameter):
    try:
        # Run the binary and capture its output
        result = subprocess.run(
            [binary_path, parameter], capture_output=True, check=True, text=True)
        return result.stdout.strip()
    except subprocess.CalledProcessError as e:
        raise RuntimeError(f"Error while executing the binary: {e}") from e


def validate_packages_json(json_data):
    # Path to the schema file
    schema_file = Path("qa", "packages_schema.json")

    # Load the schema from the file
    with open(schema_file, "r") as f:
        packages_schema = json.load(f)

    try:
        # Validate the JSON data against the schema
        validate(instance=json_data, schema=packages_schema)
    except ValidationError as e:
        # If the validation fails, print the error message
        if len(e.absolute_path) >= 3:
            pkg_name=json_data['packages'][e.absolute_path[1]]['name']
            pkg_format=json_data['packages'][e.absolute_path[1]]['format']
            pytest.fail(f"The output for package '{pkg_name}' with format '{pkg_format}' does not comply with the schema, in the field '{e.absolute_path[2]}': {e}")
        else:
            pytest.fail(f"The output does not comply with the schema: {e}")


def test_json_output():
    # Path to the shared library
    binary_filename = "sysinfo_test_tool.exe" if platform.system() == "Windows" else "sysinfo_test_tool"
    binary_path_folder = "C:\\data_provider" if platform.system() == "Windows" else "build/bin"
    binary_path = Path(binary_path_folder, binary_filename)

    # Ensure the binary exists
    assert binary_path.exists(), f"The binary is not found at {binary_path}"

    # Call the binary and get the JSON output
    output = call_binary(binary_path, "--packages")
    # Verify that the output is valid JSON
    try:
        json_data = json.loads(output)
    except json.JSONDecodeError as e:
        pytest.fail(f"The output is not valid JSON: {e}")

    # Validate that the JSON complies with the schema
    validate_packages_json(json_data)
