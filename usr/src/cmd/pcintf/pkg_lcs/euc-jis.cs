#ident	"@(#)pcintf:pkg_lcs/euc-jis.cs	1.1"
$ SCCSID(@(#)euc-jis.cs	4.1	LCC)	/* Modified: 16:21:24 8/22/89 */
$	Table euc-jap

$hexadecimal
$default_char 2a

$input
00-7f	direct char_bias 2000 
8e-8e	direct double_byte a1-df char_bias a080
b0-fe	direct double_byte a1-fe char_bias 7f80
80-ff	direct char_bias 2000


$output
2000-207f	direct_cell 
2f21-2f7e	direct_cell direct_row char_bias 5f80
3000-7f7f	direct_cell direct_row no_upper char_bias 8080
