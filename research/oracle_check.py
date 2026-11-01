"""Oracle validation: checks the naming SPEC (plan/PLAN.md §3.2) against note
tables transcribed from Xenharmonic Wiki, 'Kite's ups and downs notation',
permalink oldid=237152 (https://en.xen.wiki/index.php?title=Kite%27s_ups_and_downs_notation&oldid=237152),
section 'Examples: edos 12-24'. For flat-N edos the 'sharp raises the pitch' row is used.
This is a research artefact, NOT the product implementation."""
import math
NOM_FIFTHS = {'F':-1,'C':0,'G':1,'D':2,'A':3,'E':4,'B':5}
NOM_ORDER = "CDEFGAB"
DEGENERATE = {1,2,3,4,6,8}

def analyse(N):
    if N in DEGENERATE: return None
    f = round(N*math.log2(1.5))
    if 3*N - 5*f < 0: f -= 1           # descending minor 2nd -> second-best fifth (13b, 18b)
    s = 7*f - 4*N
    if 2*f - N <= 0 or 3*N - 5*f < 0: return None
    return f, abs(s)                     # sharp always raises by |s| (sharp-raises convention)

def pos(N,f,nom): return (NOM_FIFTHS[nom]*f) % N

def acc_str(a): return {-2:'bb',-1:'b',0:'',1:'#',2:'x'}[a]
def arrow_str(u): return ('^'*u) if u>0 else ('v'*(-u))

def naturals(N,f):
    # unwrapped positions within one octave, letter order C..B, plus C' = N
    return [('C',0),('D',2*f-N),('E',4*f-2*N),('F',N-f),('G',f),('A',3*f-N),('B',5*f-2*N),('C',N)]

def spell(N, r):
    an = analyse(N)
    if an is None: return str(r % N)
    f, s = an
    r %= N
    nat = naturals(N,f)
    lower = max(i for i,(n,p) in enumerate(nat) if p <= r)      # latest letter at/below r
    upper = min(i for i,(n,p) in enumerate(nat) if p > r)       # earliest letter above r
    if nat[lower][1] == r: return nat[lower][0]
    best=None
    for idx in (lower, upper):
        nom, p = nat[idx]
        for a in ([0] if s==0 else [-2,-1,0,1,2]):
            u = r - (p + a*s)
            key=(abs(u)+abs(a), abs(u), 0 if a>=0 else 1, 0 if u>=0 else 1, idx)
            cand=(key, arrow_str(u)+nom+acc_str(a))
            if best is None or cand<best: best=cand
    return best[1]

T = {
12:"D|D#/Eb|E|F|F#/Gb|G|G#/Ab|A|A#/Bb|B|C|C#/Db",
13:"D|E|^E/Fb|vE#/^Fb|E#/vF|F|G|A|B|^B/Cb|vB#/^Cb|B#/vC|C",
14:"D|^D/vE|E|^E/vF|F|^F/vG|G|^G/vA|A|^A/vB|B|^B/vC|C|^C/vD",
15:"D|^D|vE|E/F|^F|vG|G|^G|vA|A|^A|vB|B/C|^C|vD",
16:"D|D#/Eb|E|E#|Fb|F|F#/Gb|G|G#/Ab|A|A#/Bb|B|B#|Cb|C|C#/Db",
17:"D|^D/Eb|D#/vE|E|F|^F/Gb|F#/vG|G|^G/Ab|G#/vA|A|^A/Bb|A#/vB|B|C|^C/Db|C#/vD",
18:"D|^D/vE|E|^E|E#/Fb|vF|F|^F/vG|G|^G/vA|A|^A/vB|B|^B|B#/Cb|vC|C|^C/vD",
19:"D|D#|Eb|E|E#/Fb|F|F#|Gb|G|G#|Ab|A|A#|Bb|B|B#/Cb|C|C#|Db",
20:"D|^D|^^D/vvE|vE|E/F|^F|^^F/vvG|vG|G|^G|^^G/vvA|vA|A|^A|^^A/vvB|vB|B/C|^C|^^C/vvD|vD",
21:"D|^D|vE|E|^E|vF|F|^F|vG|G|^G|vA|A|^A|vB|B|^B|vC|C|^C|vD",
22:"D|^D/Eb|vD#/^Eb|D#/vE|E|F|^F/Gb|vF#/^Gb|F#/vG|G|^G/Ab|vG#/^Ab|G#/vA|A",
23:"D|D#|Eb|E|E#|Ex/Fbb|Fb|F|F#|Gb|G|G#|Ab|A|A#|Bb|B|B#|Bx/Cbb|Cb|C|C#|Db",
24:"D|^D/vEb|D#/Eb|^D#/vE|E|^E/vF|F|^F|F#/Gb|vG|G|^G/vAb|G#/Ab|^G#/vA|A",
}
fails=0; checked=0
for N,row in T.items():
    f,s = analyse(N); d = pos(N,f,'D')
    for i,cell in enumerate(row.split('|')):
        got = spell(N, d+i); checked+=1
        if got not in cell.split('/'):
            fails+=1; print(f"FAIL {N}edo step D+{i}: got {got!r}, wiki {cell!r}")
print(f"checked {checked} cells, {fails} failures")
# 13edo page (oldid=222544) narrow-fifth, major-wider table, note column, from D
row13 = "D|^D/E|^E/Fb|vE#/^Fb|E#/vF|F/vG|G/vA|^G/A|^A/B|^B/Cb|vB#/^Cb|B#/vC|C/vD"
f,s=analyse(13); d=pos(13,f,'D'); ok=all(spell(13,d+i) in c.split('/') for i,c in enumerate(row13.split('|')))
print("13edo page cross-check:", "PASS" if ok else "FAIL", [spell(13,d+i) for i in range(13)])
for N in (5,7,10,31,41,53,72):
    an=analyse(N); print(N, an, [spell(N,r) for r in range(N)])
print("degenerate:", [N for N in range(1,97) if analyse(N) is None])
# uniqueness property across 5..96
bad=[N for N in range(5,97) if analyse(N) and len({spell(N,r) for r in range(N)})!=N]
print("non-unique name sets:", bad)
# frequency naming vectors
def name_freq(hz,a4,N):
    an=analyse(N); f=an[0] if an else 0
    stepA = round(N*math.log2(hz/a4)); cents = 1200*math.log2(hz/a4) - stepA*1200/N
    posA = (3*f)%N if an else 0
    fromC = stepA + posA; octave = 4 + math.floor(fromC/N)
    return spell(N, fromC % N), octave, cents, stepA
for v in [(440,440,12),(261.6255653,440,12),(82.41,440,12),(82.41,440,31),(329.63,440,31),(82.41,440,13),(432,432,19),(466.1637615,440,12)]:
    print(v, name_freq(*v))
