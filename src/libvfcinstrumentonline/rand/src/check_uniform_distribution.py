import numpy as np
import sys
import plotly.express as px
from scipy.stats import binomtest


def is_valid_line(line):
    return not line.startswith('--') and not line.startswith('f32') and not line.startswith('f64')

def load_values(filename):
    with open(filename, 'r') as f:
        values =  [x.strip().split(',') for x in f.readlines() if is_valid_line(x)]
        # flatten the list of lists
        values = [float.fromhex(item.strip()) for sublist in values for item in sublist if item != '']
        return np.array(values)

def check_uniform_distribution(values, num_bins):
    hist, bin_edges = np.histogram(values, bins=num_bins)
    return hist

def main():
    filename = sys.argv[1]
    
    values = load_values(filename)
    # px.histogram(values, title='Distribution of rand').show()
    hist = check_uniform_distribution(values, 10)

    print('Distrution', hist)

    valueslog = np.log2(values)
    # px.histogram(valueslog, title='Distribution of log2(rand)').show()
    hist = check_uniform_distribution(valueslog, 10)
    
    print('Distribution log', hist)

    # plot percentage of values above 0.5
    values_above_half = values[values > 0.5]
    percentage = len(values_above_half) / len(values)
    print('Percentage of values above 0.5', percentage)
    
    # Do binomial test using scipy
    n = len(values)
    p = 0.5
    k = len(values_above_half)
    p_value = binomtest(k, n, p)
    print('P-value', p_value)
    


if __name__ == '__main__':
    main()
        