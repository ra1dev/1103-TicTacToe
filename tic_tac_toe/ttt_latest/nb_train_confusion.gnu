set terminal pngcairo size 600,500 enhanced font "Arial,14"
set output "nb_train_confusion.png"

unset key
set title "Naive Bayes Training Confusion Matrix (acc = 72.55%, err = 27.45%)"

set size ratio -1
set xrange [0.5:2.5]
set yrange [0.5:2.5]

set xtics ("Predicted +" 1, "Predicted -" 2)
set ytics ("Actual -" 1, "Actual +" 2)

set xlabel "Predicted label"
set ylabel "Actual label"

# max cell in training matrix is 440
set cbrange [0:440]
set palette rgbformulae 33,13,10

plot "nb_train_confusion.dat" using 1:2:3 with image, \
     ""                    using 1:2:4 with labels center font ",12"
