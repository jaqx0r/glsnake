"""
Converts from the format used at 
  http://home.t-online.de/home/thlet.wolter/index_en.htm
to a C array suitable for use in glsnake.c

By Andrew Bennetts, adapted from Peter Aylett's VBScript code.
"""

dirs_right = {'2': 'PIN', '1': 'RIGHT', '3': 'LEFT', '0': 'ZERO'}
dirs_left = {'2': 'PIN', '1': 'LEFT', '3': 'RIGHT', '0': 'ZERO'}

def magic(title, data):
    model = ['ZERO'] * 22

    for twist in data.split('-'):
        if len(twist) == 3:
            twist = ' ' + twist
        node = int(twist[:2])
        side = twist[2:3].upper()
        dir = twist[3:4]

        if side=='R':
            index = node * 2 - 2
        else:
            index = node * 2 - 3

        
        if side=='R':
            model[index] = dirs_right[dir]
        else:
            model[index] = dirs_left[dir]

    print "float %s[] = { %s };" % (title.lower().replace(' ', '_'),
                                    ', '.join(model))

