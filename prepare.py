import os
import re
import shutil


def merge_bikeshed_files(directory, output_file):
    file_pattern = re.compile(r'^\d+')
    files = [f for f in os.listdir(directory) if f.endswith('.bs') and file_pattern.match(f)]
    files.sort()

    print(f'Merge {len(files)} markdown files into {output_file}')

    with open(output_file, 'w') as outfile:
        for filename in files:
            filepath = os.path.join(directory, filename)
            with open(filepath, 'r') as infile:
                outfile.write(infile.read())


if __name__ == "__main__":
    directory = 'bikeshed'
    output_bs_filename = 'index.bs'

    merge_bikeshed_files(directory, output_bs_filename)

    print('Copy footer.include file')
    shutil.copyfile(os.path.join(directory, 'footer.include'), './footer.include')
