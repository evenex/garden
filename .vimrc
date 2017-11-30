"PATHOGEN
execute pathogen#infect()

" SYNTAX
syntax enable
filetype on
colorscheme solarized
set background=dark

" TRANSPARENCY
hi! Normal ctermbg=16
hi! Folded ctermbg=none

" LINE NUMBERS
set nu
hi! LineNr ctermfg=238 ctermbg=none
hi! CursorLineNr ctermfg=242 ctermbg=16

" ACTIVE BUFFER
hi! StatusLine cterm=reverse ctermfg=234 ctermbg=242
hi! StatusLineNC cterm=reverse ctermfg=233 ctermbg=237
au InsertEnter * hi! StatusLine cterm=none
au InsertEnter * set timeoutlen=0
au InsertLeave * hi! StatusLine cterm=reverse
au InsertLeave * set timeoutlen=0

" CROSSHAIR
set cursorline
set cursorcolumn

" TABS (2 SPACES)
set tabstop=2
set shiftwidth=2
set autoindent
set expandtab

" LINEBREAK
set lbr
set nowrap
set linebreak
set showbreak=⋯

" FOLDING
function! MyFoldText()
	let indent_level = indent(v:foldstart)
	let indent_str = repeat(' ', indent_level)
	let no_header = substitute(foldtext(), '[+-]\+\s*[0-9]\+\s*lines:\s','', '')
	return indent_str . no_header
endfunction
set foldtext=MyFoldText()
set foldmethod=syntax

" ACCIDENTALLY HITTING CAPITAL LETTERS
command! W write
command! Q quit

" REBUILD
command! GO wa | !./rebuild

" QUICK SCOPE
let @f = 'o{}iO	'
