import re


file_path = 'old_stuff/06.bitstream.syntax.md'


def remove_markdown_syntax(tables_md):
    cleaned_tables = []

    for table in tables_md:
        cleaned_table = []
        lines = table.split('\n')

        for line in lines:
            if line.startswith('| ----------'):
                continue
            line = line.replace('| **Type**', '').strip()
            if line.startswith('|'):
                line = line[1:]
            if line.endswith('|'):
                line = line[:-1].rstrip()
            # deal with elements and types now
            if '@@' in line:
                try:
                    idx = line.rindex('|')
                    syn_type = line[idx+1:].strip()
                    line = line[:idx].rstrip()
                    line = line.replace('@@', syn_type + ' ')
                    line = line + ';'
                except ValueError:
                    pass # no type found but I don't care
            # deal with escape symbols
            line = line.replace(r'\>\>', '>>').replace(r'\<\<', '<<').replace(r'\|\|', '||')
            line = line.replace(r'\-\-', '--').replace(r'\+\+', '++')
            line = line.replace(r'>\=', '>=').replace(r'<\=', '<=').replace(r'\|', '|')
            cleaned_table.append(line)
        entry = '\n'.join(cleaned_table)
        if entry.endswith(' }'):
            entry = entry[:-2] + '}' # fix the last }
        cleaned_tables.append(entry.strip())

    return cleaned_tables


tables_md = []
in_table = False
current_table = []

with open(file_path, 'r') as file:
    content = file.read()

for line in content.splitlines():
    if line.startswith('|') and '{:.syntax }' not in line:
        in_table = True
        current_table.append(line)
    elif '{:.syntax }' in line and in_table:
        in_table = False
        tables_md.append('\n'.join(current_table))
        current_table = []
    elif in_table:
        current_table.append(line)

print(f'Found {len(tables_md)} syntax elements')
clean_tables = remove_markdown_syntax(tables_md)

for table in clean_tables:
    print(table)
    print('\n\n')
