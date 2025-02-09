import json
import re

TEXT_CODES = {'bold': {'start': '\x1b[1m',
                       'end': '\x1b[22m'},
              'cyan': {'start': '\x1b[36m',
                       'end': '\x1b[39m'},
              'blue': {'start': '\x1b[34m',
                       'end': '\x1b[39m'},
              'red': {'start': '\x1b[31m',
                      'end': '\x1b[39m'},
              'magenta': {'start': '\x1b[35m',
                          'end': '\x1b[39m'},
              'green': {'start': '\x1b[32m',
                        'end': '\x1b[39m'},
              'yellow': {'start': '\x1b[33m',
                         'end': '\x1b[39m'},
              'underline': {'start': '\x1b[4m',
                            'end': '\x1b[24m'}}


def bold(text):
    return text_attribute(text, 'bold')


def cyan(text):
    return text_attribute(text, 'cyan')


def blue(text):
    return text_attribute(text, 'blue')


def red(text):
    return text_attribute(text, 'red')


def magenta(text):
    return text_attribute(text, 'magenta')


def green(text):
    return text_attribute(text, 'green')

def yellow(text):
    return text_attribute(text, 'yellow')

def underline(text):
    return text_attribute(text, 'underline')


def text_attribute(text, attribute):
    return "{start}{txt}{stop}".format(start=TEXT_CODES[attribute]['start'],
                                       txt=text,
                                       stop=TEXT_CODES[attribute]['end'])


FUNC_DICT = {'blue': blue,
             'bold': bold,
             'green': green,
             'yellow': yellow,
             'cyan': cyan,
             'magenta': magenta,
             'underline': underline,
             'red': red}


def format_text(text, *args):
    return_string = text
    for i in args:
        func = FUNC_DICT.get(i)
        if func:
            return_string = func(return_string)
    return return_string

# pretty print for JSON
def pretty_json (json_str, use_colors = True):
    pretty_str = json.dumps(json.loads(json_str), indent = 4, separators=(',', ': '), sort_keys = True)

    if not use_colors:
        return pretty_str

    try:
        # int numbers
        pretty_str = re.sub(r'([ ]*:[ ]+)(\-?[1-9][0-9]*[^.])',r'\1{0}'.format(blue(r'\2')), pretty_str)
        # float
        pretty_str = re.sub(r'([ ]*:[ ]+)(\-?[1-9][0-9]*\.[0-9]+)',r'\1{0}'.format(magenta(r'\2')), pretty_str)
        # # strings
        #
        pretty_str = re.sub(r'([ ]*:[ ]+)("[^"]*")',r'\1{0}'.format(red(r'\2')), pretty_str)
        pretty_str = re.sub(r"('[^']*')", r'{0}\1{1}'.format(TEXT_CODES['magenta']['start'],
                                                             TEXT_CODES['red']['start']), pretty_str)
    except :
        pass

    return pretty_str


if __name__ == "__main__":
    pass
