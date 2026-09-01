set nocompatible

syntax on
colorscheme morning

set encoding=utf-8
set number
set ruler
set cursorline
set cursorcolumn
set showmatch
set statusline=\ %F%m%r%h\ %w\ \ CWD:\ %r%{getcwd()}%h\ \ \ Line:\ %l\ \ Column:\ %c
set laststatus=2

set tabstop=4
set softtabstop=4
set shiftwidth=4
set autoindent
set smartindent
set expandtab

let mapleader=" "
nmap <leader>q :wq!<cr>

inoremap jk <esc>

if has('clipboard')
    set clipboard=unnamedplus,unnamed
endif
