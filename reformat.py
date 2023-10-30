import os
import re

directory = os.getcwd()
text_file_pattern = r'.*\.hpp$'

for filename in os.listdir(directory):
  if re.match(text_file_pattern, filename):
    input_file_path = os.path.join(directory, filename)

    with open(input_file_path, 'r') as file:
      file_contents = file.read()

    updated_contents = file_contents.replace('.h"', '.hpp"')

    with open(input_file_path, 'w') as file:
      file.write(updated_contents)