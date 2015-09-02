# Regression resting some situtaions
./rogo <<INPUT | tee temp2
#play_seq D4 E3 E4 D3 C3 C2 F3 C4 B3 B4 B2 F2 D2 E2 C5 D1 B5 F4 F5 G3
#query expect G5
#clear_board
play_seq A4 A5 B4 B5 C4 C5 D4 D5 E4 E5
query expect F5
clear_board
play_seq D4 D3 C4 C3 E3 E2
query expect B3 F3 F2
clear_board
query expect D4
quit
INPUT

grep Result temp2
rm temp
