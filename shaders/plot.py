import numpy as np
import matplotlib.pyplot as plt
import os

# Define the filenames and their labels
files_and_labels = [
    ("angularDifferencesReflect.txt", "Reflected Normal"),
    ("angularDifferencesExtrapolation.txt", "Extrapolated Normal"),
    ("angularDifferencesNew.txt", "New Method")
]

data_list = []
labels = []

# Load data from all files
for fname, label in files_and_labels:
    if not os.path.isfile(fname):
        raise FileNotFoundError(f"Cannot find '{fname}' in {os.getcwd()}")
    
    data = np.loadtxt(fname, comments='#', usecols=1)
    data_list.append(data)
    labels.append(label)

# Plot comparison histogram
plt.figure(figsize=(10, 6))
plt.hist(data_list,
         bins=50,
         label=labels,
         alpha=0.7,
         edgecolor='black')

plt.xlabel('Angular Deviation (degrees)')
plt.ylabel('Frequency')
plt.title('Angular Deviation: Reflect vs Extrapolate vs New')
plt.legend()
plt.tight_layout()

# Save and show
plt.savefig('angular_comparison_histogram.png', dpi=300)
plt.show()
