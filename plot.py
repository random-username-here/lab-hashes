import pandas as pd
import matplotlib.pyplot as plt

df = pd.read_csv(
    'res/time.tsv',
    sep='\t',
    names=['version', 'run', 'time']
)

plt.xlabel('Version')
plt.ylabel('Time')
plt.title('Time for diffrent versions to execute a lot of searches')
plt.scatter(df['version'], df['time'])

plt.xticks(range(0, 5), [
    'Initial -O3',
    'Initial',
    'strcmp',
    'crc32',
    'htab_get'
])

plt.savefig('plot.svg')
