clang -O3 -I include main.c include/lexer.c include/error.c include/token.c include/utils.c include/ast.c include/parser.c include/analyzer.c include/bytecode.c include/bytecode_compiler.c include/vm.c -o ./bin/cimpl -g
