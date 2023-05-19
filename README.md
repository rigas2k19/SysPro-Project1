sdi1900019 Vasileiou Riginos


# Sniffer 

Compile command : make all
How to run : ./sniffer (for current directory)
             ./sniffer -p path (for directory in path)
Clean : make clean

Θα χρειαστεί να δημιουργήσετε έναν κενό φάκελο output.
Αν θέλουμε να ξανατρέξουμε το πρόγραμμα πρέπει να αδειάσουμε το output/ από τα .out αρχεία.

ABOUT THE CODE

Όλα τα reads γίνονται με BUFFSIZE και όχι δυναμικά
Ο φάκελος output/ χρειάζεται για την εργασία. Αρχικά είναι κενός, μετά θα μπουν τα .out αρχεία
Ο φάκελος txt/ έχει τα δοκιμαστικά αρχεία που μας δόθηκαν

1) ADTList.c : Ο κώδικάς μου για λίστες από τις Δομές Δεδομένων, άλλαξα λίγο τα ListNodes ώστε κάθε ListNode 
να έχει έναν counter και να τον κάνει update για κάθε url που κάνουμε insert ενώ υπάρχει ήδη στη λίστα. 

2) ADTQueue.c : Ο κώδικας για τη δομή Queue 

common_types.h header file στο οποίο υπάρχουν οι ορισμοί για τους κοινούς τύπους πχ. void* = Pointer

3) listener.c : Η συνάρτηση που καλείται από τον manager και κάνει execvp() την inotifywait

4) manager.c : O manager. Επικοινωνεί με τον listener μέσω pipe. Παίρνει το path για τον φάκελο που θέλουμε να παρακολουθεί
η inotifywait και το βάζει στη μεταβλητή mypath για να καλέσει τον listener με τα κατάλληλα ορίσματα. Αν δεν περάσουμε όρισμα 
στην κλήση τότε ο listener θα παρακολουθεί το current directory. Φτιάχνουμε το pipe και συνδέουμε την έξοδο του listener με 
αυτό. O manager τώρα πρέπει να διαβάσει από τον listener το αρχείο και το path που στέλνει η inotifywait. 
Κάθε φορά που βρίσκει ένα νέο αχείο από τον listener, ενημερώνει έναν worker για το αρχείο αυτό και ο worker το επεξεργάζεται.
Αν δεν υπάρχει κανένας worker σταματημένος τότε δημιουργεί μια νέα διεργασία worker, αλλιώς ελέγχει την ουρά στην οποία κρατάμε 
τους σταματημένους workers και στέλνει σήμα SIGCONT στην πρώτη διεργασία worker για το αρχείο αυτό.
O manager επικοινωνεί με τον worker μέσω named pipe. Τα named pipes έχουν ονόματα του τύπου "/tmp/fifo.(process_id)". O manager 
γράφει στο named pipe και o worker διαβάζει από εκεί. O worker καλείται και αυτός με execvp().

## Σήματα στον manager 
O manager τρέχει σε μια λούπα η οποία διακόπτεται από σήμα SIGINT. Στη while αυτή έχω μια global μεταβλητή end η οποία αλλάζει 
από τον handler του SIGINT. Αρχικά η end είναι 1 (true) και όταν δοθεί το σήμα SIGINT από τον χρήστη αλλάζει σε 0 (false) και  
φεύγει από την επανάληψη. Αφού φύγει κάνω free την queue για τους διαθέσιμους workers. Οι υπόλοιπες
μεταβλητές που έχουν γίνει malloc από τον manager γίνονται free στη while. Όταν δοθεί SIGINT o handler μαζέυει όλους 
τους σταματημένους workers και στέλνει σήμα SIGKILL για να τους τερματίσει. (Η μνήμη που χρησιμοποιείται από τους workers 
γίνεται free πριν στείλουν σήμα raise(SIGSTOP) στον εαυτό τους).
Όταν οι workers κάνουν raise(SIGSTOP), o manager δέχεται σήμα SIGCHLD. Έχω λοιπόν έναν handler για αυτό ο οποίος αλλάζει μια
μεταβλητή workerAvailable από false σε true κάθε φορά που υπάρχει σταματημένος worker.
## Άλλες συναρτήσεις του manager
Υπάρχουν άλλες 3 συναρτήσεις στον manager οι οποίες παίρνουν τα ονόματα των files και pipes αντίστοιχα και μια για δημιουργία 
int* από ακέραιο (για την εισαγωγή του pid στην queue). Το πως δουλεύουν οι αλγόριθμοι εξηγείται στα σχόλια του κώδικα.

5) worker.c : O worker διαβάζει από το named pipe το filename που του στέλνει ο manager. Ανοίγει το file και διαβάζει ψάχνοντας 
για urls. Ο αλγόριθμος που ξεχωρίζει τα locations από το url εξηγείται στα σχόλια του κώδικα. Για κάθε location που βρίσκει
το βάζει σε μια λίστα. Αν βρει ένα location που υπάρχει ήδη στη λίστα, τότε αυξάνει τον counter με τον αριθμό εμφανίσεών του
κατά 1 (στο ListNode γίνεται αυτό). Τα .out αρχεία μπαίνουν στον φάκελο output/ . Πριν δημιουργήσουμε το .out βγάζουμε από το
filename το path και κρατάμε μόνο το όνομα του αρχείου. Στο τέλος του ονόματος βάζουμε .out. Τέλος βάζουμε το αρχείο στον  
φάκελο. Τέλος για να γεμίσουμε το <filename>.out διατρέχουμε τη λίστα με τα locations και τα γράφουμε στο αρχείο. Κάνουμε free
τη λίστα στο τέλος της while και μετά raise(SIGSTOP) για να σταματήσει και να στείλει σήμα SIGCHLD στον manager.

# Finder 

first run : chmod u+x finder.sh
then run : ./finder.sh [args]       (px. ./finder.sh com gr gov)

Το πώς λειτουργεί ο finder εξηγείται στα σχόλια. Ουσιαστικά διατρέχει τα .out για κάθε ένα από τα arguments που του περνάμε,
τα διαβάζει γραμμή-γραμμή και ψάχνει να βρει αν υπάρχει location με tld αυτο που του περνάμε σαν όρισμα (Στο παράδειγμα ψάχνει να βρει location
που τελειώνει σε com, gr και gov). Χωρίζει την κάθε γραμμή στο " ". Tο πρώτο token είναι το location και το δεύτερο είναι ο counter.
Αν το location τελειώνει στο συγκεκριμένο tld τότε αυξάνει τον αριθμό εμφανίσεων του location κατά counter.
