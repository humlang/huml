$recorder = 1;
$pdf_mode = 1;
$pdflatex = "pdflatex -shell-escape -interaction=nonstopmode %O %S";
$out_dir = 'build';
$bibtex_use = 2;
$pdf_previewer = 'start zathura %O %S';
@default_files = ('grammar.tex');
