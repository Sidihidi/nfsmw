import os
import json,sys
SCR = os.environ.get('SCRATCH', os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'build', 'scratch'))
os.makedirs(SCR, exist_ok=True)
unit=sys.argv[1]
d=json.load(open(f'{SCR}/d_{unit}.json'))
L={s['name']:s for s in d['left']['symbols'] if s.get('kind')=='SYMBOL_FUNCTION'}
R={s['name']:s for s in d['right']['symbols'] if s.get('kind')=='SYMBOL_FUNCTION'}
rows=[]
for n,s in L.items():
    mp=s.get('match_percent') or 0
    if not (60<=mp<100): continue
    li=s.get('instructions',[]); ri=(R.get(n) or {}).get('instructions',[])
    bad=0; ins=0; dele=0; rep=0
    for e in li+ri:
        k=e.get('diff_kind','')
        if k and k!='DIFF_NONE':
            bad+=1
            if k=='DIFF_INSERT': ins+=1
            elif k=='DIFF_DELETE': dele+=1
            elif k=='DIFF_REPLACE': rep+=1
    rows.append((bad,int(s['size']),mp,n,ins,dele,rep,len(li),len(ri)))
rows.sort(key=lambda r:(r[0]/max(r[1],1)))
print('%5s %7s %7s %4s %4s %4s  %s'%('bad','size','pct','ins','del','rep','name'))
for bad,sz,mp,n,ins,dele,rep,nl,nr in rows[:45]:
    print('%5d %7d %7.2f %4d %4d %4d  %s'%(bad,sz,mp,ins,dele,rep,n[:78]))
