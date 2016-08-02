import functools

def human_print_gpio(data:bytes):
    dir = {0:'INPUT', 1:'OUTPUT'}
    for i in range(0,5):
        print( 'GPIO'+str(i), data[0] >> i & 1, dir[data[1] >> i & 1] )
    for i,p in enumerate(('A','B','C','D','E','F','G', 'H')):
        print( 'GPIO'+p, data[0] >> i & 1, dir[data[1] >> i & 1] )