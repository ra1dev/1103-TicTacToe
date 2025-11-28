set terminal pngcairo size 800,600
set output 'nb_confusion_matrix_test.png'

set title 'Naive Bayes Confusion Matrix (Test Set - Raw Counts)'
set xlabel 'Predicted outcome for X'
set ylabel 'Actual outcome for X'

# 0 = Win, 1 = No win on both axes
set xtics ('Win' 0, 'No win' 1)
set ytics ('Win' 0, 'No win' 1)

set cblabel 'Number of examples'
set cbrange [0:110]          # max a bit above your largest cell (103)

set xrange [-0.5:1.5]
set yrange [1.5:-0.5]        # flip Y so (0,0) is top-left like a matrix

set grid front lc rgb '#444444'
set palette rgbformulae 22,13,-31   # nice purple–yellow style

unset key

plot 'confusion_test.dat' using 1:2:3 with image, \
     '' using 1:2:(sprintf('%d', $3)) with labels center tc rgb 'white' font ',14'
