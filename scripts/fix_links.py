import os
import re

BS_FILES = [
    '01_Scope.bs',
    '02_Terms_and_definitions.bs',
    '03_Symbols_and_abbreviated_terms.bs',
    '04_Conventions.bs',
    '05_Syntax_structures.bs',
    '06_Syntax_structures_semantics.bs',
    '07_Decoding_process.bs',
    '08_Parsing_process.bs',
    '09_Additional_tables.bs',
    '10_Annex_A_Profiles_and_levels.bs',
    '11_Annex_B_Length_delimited_bitstream_format.bs',
    '12_Annex_C_Error_resilience_behavior_inf.bs',
    '13_Annex_D_Large_scale_tile_use_case_inf.bs',
    '14_Annex_E_Decoder_model.bs',
    '15_Bibliography.bs'
]


# Function to parse the headings in a bikeshed file and construct the table of contents
def parse_headings(file_content):
    heading_pattern = re.compile(r'^(#+) (.+) {#([^}]+)}$', re.MULTILINE)
    headings = heading_pattern.findall(file_content)

    toc = {}
    numbering = []

    for heading in headings:
        level = len(heading[0])
        title = heading[2]

        while len(numbering) < level:
            numbering.append(0)
        while len(numbering) > level:
            numbering.pop()
        numbering[-1] += 1

        toc_key = '.'.join(map(str, numbering))
        toc[toc_key] = f'#{title}'

    return toc


# Function to replace empty markdown links with toc IDs
def replace_empty_links(file_content, toc):
    link_pattern = re.compile(r'\[section ([\d\.]+)\]\[\]')

    def replace_link(match):
        section = match.group(1)
        return f'[[{toc.get(section, "UNKNOWN")}]])'

    return link_pattern.sub(replace_link, file_content)


# Process each file
for bs_file in BS_FILES:
    with open(bs_file, 'r') as file:
        file_content = file.read()

    # Parse headings to construct toc
    toc = parse_headings(file_content)

    # Replace empty markdown links with toc IDs
    updated_content = replace_empty_links(file_content, toc)

    # Write the updated content back to the file
    # with open(bs_file, 'w') as file:
    #     file.write(updated_content)

    # print(f"Processed {bs_file}")
