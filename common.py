import subprocess
import json

def parse(filename):
    config=json.load(open(filename))
    functable=[]
    spec_functable=[]
    for i in config['functable']:
        if i['mask']:
            functable.append((i['name'],i['type'],i['skb'],i['sk']))
    for i in config['special_functable']:
        if i['mask']:
            spec_functable.append((i['name'],i['type'],i['skb'],i['sk']))
    return functable, spec_functable

def execute(commands):
    process = subprocess.Popen('/bin/bash', stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    out, err = process.communicate(commands)
    return out,err

if __name__=='__main__':
    ft,spt=parser('config.json')
    print ft
    print spt