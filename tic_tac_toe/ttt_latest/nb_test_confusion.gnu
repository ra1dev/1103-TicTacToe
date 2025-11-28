set terminal pngcairo size 600,500 enhanced font "Arial,14"
set output "nb_test_confusion.png"

unset key
set title "Naive Bayes Testing Confusion Matrix (acc = 70.47%, err = 29.53%)"

set size ratio -1
set xrange [0.5:2.5]
set yrange [0.5:2.5]

set xtics ("Predicted +" 1, "Predicted -" 2)
set ytics ("Actual -" 1, "Actual +" 2)

set xlabel "Predicted label"
set ylabel "Actual label"

# IMPORTANT: same colour range & palette as training plot
set cbrange [0:440]             # max from training matrix
set palette rgbformulae 33,13,10

plot "nb_test_confusion.dat" using 1:2:3 with image, \
     ""                    using 1:2:4 with labels center font ",12"
