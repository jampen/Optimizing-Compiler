from pprint import pprint
from graphviz import Digraph
from sys import argv

def make_blank_bb():
    return {
        'successors' : [],
        'label': None,
        'src': []
    }

def parse(lines):
    blocks = {}
    label_to_block = {}
    current_bb = None

    for line in lines:
        line = line.strip()
        if line.startswith("BB"):
            current_bb = line.split(':')[0]
            blocks[current_bb] = make_blank_bb()
            continue
        elif line.startswith('j'):
            target = line.split()[1]
            blocks[current_bb]["successors"].append(target)
        elif line.startswith('b'):
            b_v, iftrue, iffalse = line.split(',')
            iftrue = iftrue.lstrip()
            iffalse = iffalse.lstrip()
            b, v = b_v.split(' ')
            blocks[current_bb]["successors"].append(iftrue)
            blocks[current_bb]["successors"].append(iffalse)
        elif line.startswith('L'):
            if blocks[current_bb]['label'] is None:
                l = line.split(':')[0].lstrip()
                blocks[current_bb]['label'] = l
                label_to_block[l] = current_bb

        if current_bb:
            blocks[current_bb]['src'].append(line)


    # convert successors to blocks
    for bb in blocks.values():
        s = bb["successors"]
        s = [label_to_block[x] for x in s]
        bb["successors"] = s

    return blocks

def render(outname, blocks):
    font = 'IBM Plex Mono'

    dot = Digraph(
        name = "CFG",
        format = "png",
        graph_attr = {"rankdir" : "TB"},
        node_attr = {
            "shape" : "box",
            "fontname" : font ,
            "fontsize": "18"
        },
        edge_attr = {"fontname" : font }
    )

    # Create nodes
    for name, bb in blocks.items():
        src = bb["src"]
        src = [f'{x}\\l' for x in src]
        src = ''.join(src)
        dot.node(name, src)

    # Build edges
    for name, bb in blocks.items():
        s = bb['successors']


    # Build edges
    for name, bb in blocks.items():
        s = bb['successors']
        l = len(s)

        if l ==2:
            dot.edge(name, s[0], color='darkgreen')
            dot.edge(name, s[1], color='darkred')
        elif l == 1:
            dot.edge(name, s[0], color='black', weight='0')

            
    dot.render(outname, cleanup=True)

    dot.render(outname, cleanup=True)


def main():
    if len(argv) != 2:
        return

    inf = argv[1]

    if not inf.endswith('.ir'):
        print("invalid file extension")
        return

    outf = f'{inf}.graph'

    try:
        with open(inf) as file:
            lines = file.readlines()
            graph = parse(lines)    
            render(outf, graph)
    except FileNotFoundError as e:
        print(e)
    except:
        raise e

if __name__ == '__main__':
    main()