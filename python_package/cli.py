"""
CLI demo for braw_metadata library.
"""
import os
import json
from pathlib import Path
import click
from braw_metadata import bmd_metadata


def find_files(path, extensions, ignore_multipart=False):
    file_paths = []
    for root, dirs, files in os.walk(path):
        for file in files:
            if file.endswith(tuple(extensions)):
                file_paths.append(os.path.join(root, file))
                if ignore_multipart:
                    break
    if not file_paths:
        raise FileNotFoundError(f"No files with the extensions {' '.join(extensions)} were found.")
    return file_paths

@click.command()
@click.option('--inputpath', '-i',
              help='Path to the directory containing the files to be processed.',
              prompt='Path to the directory containing the files to be processed')
@click.option('--verbose', '-v',
                help='Verbose output.'
                ,is_flag=True)
@click.option('--outputpath', '-o',
                help='Path to the output file. If not specified, pwd.')
def cli(inputpath, verbose, outputpath):
    if outputpath is None:
        outputpath = os.getcwd()
    if not os.path.isdir(outputpath):
        os.makedirs(outputpath, exist_ok=True)

    extensions = [".braw"]
    try:
        file_paths = find_files(inputpath.strip('\'').strip('\"'), extensions)
    except FileNotFoundError as e:
        click.echo(e)
        return

    for file_path in file_paths:

        format = "json"
        filename = os.path.basename(file_path)
        outputfile = os.path.join(outputpath, f'{filename}.{format}')

        metadata = bmd_metadata.read_metadata(file_path)
        if verbose:
            click.echo(f"Processing {file_path}")
            for key, value in metadata.items():
                click.echo(f"{key}: {value}")

        with open(outputfile, 'w') as file:
            click.echo(f"Writing to {outputfile}")
            json.dump(metadata, file, indent=4)

if __name__ == '__main__':
    cli()