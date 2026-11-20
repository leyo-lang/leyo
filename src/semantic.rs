use crate::parser::{Program, Target};
use std::collections::HashMap;

pub enum Type {
    Integer,
    Float,
    Bool,
    String,
    Array(Box<Type>),
    Function(Vec<Type>, Box<Type>),
    Void,
    Unknown,
}

pub enum SymbolKind {
    Variable,
    Constant,
    Function,
    Parameter,
}

pub struct Symbol {
    pub name: String,
    pub ty: Type,
    pub kind: SymbolKind,
    pub mutable: bool,
    pub s: usize,
    pub e: usize,
}

pub enum TypedExpression {
    Error,

    Atom(crate::lexer::Token, Type),

    Binary(
        Box<TypedExpression>,
        crate::lexer::Token,
        Box<TypedExpression>,
        Type,
    ),

    Unary(crate::lexer::Token, Box<TypedExpression>, Type),

    Array(Vec<TypedExpression>, Type),

    Call(Box<TypedExpression>, Vec<TypedExpression>, Type),

    Index(Box<TypedExpression>, Box<TypedExpression>, Type),

    Property(Box<TypedExpression>, Box<TypedExpression>, Type),

    NamespaceProperty(Box<TypedExpression>, Box<TypedExpression>, Type),
}

pub enum TypedStatement {
    None,
    End,
    Error,

    Block(Option<String>, Vec<TypedStatement>),

    VariableDeclaration(Target, TypedExpression),
    ConstantDeclaration(Target, TypedExpression),

    Assignment(Target, TypedExpression),
    Evaluation(TypedExpression),

    Loop(TypedExpression, Box<TypedStatement>),

    If(
        TypedExpression,
        Box<TypedStatement>,
        Option<Box<TypedStatement>>,
    ),

    Repeat(TypedExpression, Option<Target>, Box<TypedStatement>),

    Break,
    Continue,

    Function(String, Vec<TypedExpression>, Box<TypedStatement>),

    Return(Option<TypedExpression>),
}

pub type TypedProgram = Vec<TypedStatement>;

pub struct Scope {
    symbols: HashMap<String, Symbol>,
}

impl Scope {
    pub fn new() -> Self {
        Self {
            symbols: HashMap::new(),
        }
    }

    pub fn define(&mut self, name: String, symbol: Symbol) -> bool {
        if self.symbols.contains_key(&name) {
            return false;
        }

        self.symbols.insert(name, symbol);
        true
    }

    pub fn get(&self, name: &str) -> Option<&Symbol> {
        self.symbols.get(name)
    }
}

struct Analyser {
    ast: Program,
    result: TypedProgram,
    source_id: usize,
    scopes: Vec<Scope>,
}

impl Analyser {
    fn new(program: Program, id: usize) -> Self {
        return Analyser {
            ast: program,
            result: TypedProgram::new(),
            source_id: id,
            scopes: Vec::new(),
        };
    }

    fn finish(self) -> TypedProgram {
        self.result
    }

    fn get_symbol(&self, name: &str) -> Option<&Symbol> {
        self.scopes.iter().rev().find_map(|scope| scope.get(name))
    }

    fn define_symbol(&mut self, symbol: Symbol) -> bool {
        if let Some(scope) = self.scopes.last_mut() {
            let name = symbol.name.clone();
            scope.define(name, symbol)
        } else {
            false
        }
    }

    fn enter_scope(&mut self) {
        self.scopes.push(Scope::new());
    }

    fn exit_scope(&mut self) {
        self.scopes.pop();
    }

    fn analyse_scope(&mut self) -> Vec<TypedStatement> {
        let result = Vec::<TypedStatement>::new();
        self.enter_scope();

        self.exit_scope();
        result
    }
}

pub fn analyse(program: Program, id: usize) -> TypedProgram {
    let mut analyser = Analyser::new(program, id);
    analyser.result = analyser.analyse_scope();
    analyser.finish()
}
