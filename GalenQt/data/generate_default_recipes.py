#!/usr/bin/python -u

import sys
import os
import re
import argparse

def generate_default_recipes():

    parser = argparse.ArgumentParser(description='Produce the default recipe file')
    parser.add_argument("-b", "--builder_data", default = 'MatlabRecipeBuilderData.txt', type=str, help="the data about each recipe")
    parser.add_argument("-m", "--model_data", default = 'model_matlab_recipe.txt', type=str, help="the master recipe file")
    parser.add_argument('-o', '--output_file', default = 'DefaultRecipes.txt', type=str, help='the output file')
    parser.add_argument('-v', '--verbose', action="store_true", help='switch to verbose mode')
    args = parser.parse_args()

    if (args.verbose): print(args)

    builder_data = read_file(args.builder_data)
    model_data = read_file(args.model_data)
    output_file = open(args.output_file, 'w')

    output_file.write('# GalenQt recipe file generated by generate_default_recipes.py\n\n');

    builder_data_lines = builder_data.split('\n')
    for i in range(1, len(builder_data_lines)):
        tokens = builder_data_lines[i].split('\t')
        print(tokens)
        if len(tokens) < 9: continue
        
        description = tokens[0]
        type = tokens[1]
        supervised = mk_int(tokens[2])
        linear = mk_int(tokens[3])
        out_of_sample = mk_int(tokens[4])
        subsamples = mk_int(tokens[5])
        extra_arguments_1 = tokens[6]
        extra_arguments_2 = tokens[7]
        extra_arguments_3 = tokens[8]
        
        no_braces = type[type.find("{")+1:type.rfind("}")]
        comma_split = [x.strip() for x in no_braces.split(',')]
        first_code = comma_split[0]
        code = first_code[first_code.find("'")+1:first_code.rfind("'")]
    
        print('description', description, 'code', code)
        
        list_of_old_strings = ['TITLE_BOOKMARK','TECHNIQUE_CODE_BOOKMARK','SUPERVISED_BOOKMARK','LINEAR_BOOKMARK',
                               'OUT_OF_SAMPLE_BOOKMARK','SUBSAMPLES_BOOKMARK']
        list_of_new_strings = [description,code,supervised,linear,out_of_sample,subsamples]
        output_data = replace_list(model_data, list_of_old_strings, list_of_new_strings)
        output_file.write(output_data)
        
    output_file.close()

def replace_list(master_string, list_of_old_strings, list_of_new_strings):
    output = master_string
    for i in range(0, len(list_of_old_strings)):
        output = output.replace(str(list_of_old_strings[i]), str(list_of_new_strings[i]))
    return output

def read_file(filename):
    input = open(filename, 'r')
    output = input.read()
    input.close()
    return output

def mk_int(s):
    s = s.strip()
    return int(s) if s else 0
    
if __name__ == '__main__':
    generate_default_recipes()
