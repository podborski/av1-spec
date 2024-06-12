import os
import re


def merge_bikeshed_files(directory, output_file):
    file_pattern = re.compile(r'^\d+')
    files = [f for f in os.listdir(directory) if f.endswith('.bs') and file_pattern.match(f)]
    files.sort()

    files.insert(0, 'index_header.bs')

    with open(output_file, 'w') as outfile:
        for filename in files:
            print(f'Merge {filename}')
            filepath = os.path.join(directory, filename)
            with open(filepath, 'r') as infile:
                outfile.write(infile.read())
                # outfile.write('\n\n')


if __name__ == "__main__":
    directory = 'bikeshed'
    output_bs_filename = 'index.bs'

    merge_bikeshed_files(directory, output_bs_filename)
