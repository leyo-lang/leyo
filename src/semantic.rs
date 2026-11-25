use crate::{error, parser::Program, warn};
use std::collections::HashMap;

#[derive(PartialEq, Clone)]
pub enum Type {
    Integer,
    Float,
    Bool,
    String,
    Character,
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
    pub usages: u8,
    pub s: usize,
    pub e: usize,
}

#[derive(PartialEq)]
pub enum TypedExpressionKind {
    Error,
    Atom(crate::lexer::Token),
    Binary(
        Box<TypedExpression>,
        crate::lexer::Token,
        Box<TypedExpression>,
    ),
    Unary(crate::lexer::Token, Box<TypedExpression>),
    Array(Vec<TypedExpression>),
    Call(Box<TypedExpression>, Vec<TypedExpression>),
    Index(Box<TypedExpression>, Box<TypedExpression>),
    Property(Box<TypedExpression>, Box<TypedExpression>),
    NamespaceProperty(Box<TypedExpression>, Box<TypedExpression>),
}

#[derive(PartialEq)]
pub struct TypedExpression {
    t: Type,
    k: TypedExpressionKind,
    s: usize,
    e: usize,
}
impl TypedExpression {
    fn new(t: Type, k: TypedExpressionKind, s: usize, e: usize) -> Self {
        TypedExpression { t, k, s, e }
    }

    const fn err() -> Self {
        Self {
            t: Type::Unknown,
            k: TypedExpressionKind::Error,
            s: 0,
            e: 0,
        }
    }
}

pub enum TypedStatementKind {
    None,
    End,
    Error,
    Block(Option<String>, Vec<TypedStatement>),
    VariableDeclaration(TypedExpression, TypedExpression),
    ConstantDeclaration(TypedExpression, TypedExpression),
    Assignment(TypedExpression, TypedExpression),
    Evaluation(TypedExpression),
    Loop(TypedExpression, Box<TypedStatement>),
    If(
        TypedExpression,
        Box<TypedStatement>,
        Option<Box<TypedStatement>>,
    ),
    Repeat(
        TypedExpression,
        Option<TypedExpression>,
        Box<TypedStatement>,
    ),
    Break,
    Continue,
    Function(String, Vec<TypedExpression>, Box<TypedStatement>),
    Return(Option<TypedExpression>),
}

pub struct TypedStatement {
    k: TypedStatementKind,
    s: usize,
    e: usize,
}
impl TypedStatement {
    fn new(k: TypedStatementKind, s: usize, e: usize) -> Self {
        Self { k, s, e }
    }

    const fn err() -> Self {
        Self {
            k: TypedStatementKind::Error,
            s: 0,
            e: 0,
        }
    }
}

pub type TypedProgram = Vec<TypedStatement>;

pub struct Scope {
    id: usize,
    parent: Option<usize>,
    symbols: HashMap<String, Symbol>,
    children: HashMap<String, usize>,
}

impl Scope {
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
    result: TypedProgram,
    source_id: usize,
    scopes: Vec<Scope>,
    scope_stack: Vec<usize>,
}

impl Analyser {
    fn new(id: usize) -> Self {
        return Analyser {
            result: TypedProgram::new(),
            source_id: id,
            scopes: Vec::new(),
            scope_stack: Vec::new(),
        };
    }

    fn finish(self) -> TypedProgram {
        self.check_symbols_used();
        self.result
    }

    fn get_symbol(&self, name: &str) -> Option<&Symbol> {
        let mut current = *self.scope_stack.last()?;

        loop {
            let scope = &self.scopes[current];

            if let Some(symbol) = scope.symbols.get(name) {
                return Some(symbol);
            }

            current = scope.parent?;
        }
    }

    fn expression_to_path(&self, expr: &crate::parser::Expression) -> Option<Vec<String>> {
        use crate::parser::ExpressionKind::*;

        Some(match &expr.k {
            Atom(token) => vec![token.v.clone()],

            NamespaceProperty(lhs, rhs) | Property(lhs, rhs) => {
                let mut path = self.expression_to_path(&**lhs)?;
                path.extend(self.expression_to_path(&**rhs)?);
                path
            }

            _ => {
                error!("Is Not Valid Path", self.source_id, expr.s, expr.e);
                return None;
            }
        })
    }

    fn resolve_symbol(&self, expr: crate::parser::Expression) -> Option<&Symbol> {
        let path = self.expression_to_path(&expr)?;
        let (first, rest) = path.split_first()?;

        let mut current = *self.scope_stack.last()?;

        loop {
            let scope = &self.scopes[current];

            if scope.symbols.contains_key(first) || scope.children.contains_key(first) {
                break;
            }

            current = scope.parent?;
        }

        for part in rest.iter().take(rest.len().saturating_sub(1)) {
            current = *self.scopes[current].children.get(part)?;
        }

        self.scopes[current]
            .symbols
            .get(rest.last().unwrap_or(first))
    }

    fn define_symbol(&mut self, symbol: Symbol) -> bool {
        if let Some(scope) = self.scopes.last_mut() {
            let name = symbol.name.clone();
            scope.define(name, symbol)
        } else {
            false
        }
    }

    fn create_scope(&mut self, parent: Option<usize>) -> usize {
        let id = self.scopes.len();

        self.scopes.push(Scope {
            id,
            parent,
            symbols: HashMap::new(),
            children: HashMap::new(),
        });

        id
    }

    fn enter_scope(&mut self, id: usize) {
        self.scope_stack.push(id);
    }

    fn exit_scope(&mut self) {
        self.scope_stack.pop();
    }

    fn check_symbols_used(&self) {
        for scope in &self.scopes {
            for (_, symbol) in &scope.symbols {
                if symbol.usages == 0 {
                    warn!("Unused Symbol", self.source_id, symbol.s, symbol.e)
                }
            }
        }
    }

    fn is_targetable(&self, texpr: &TypedExpression) -> bool {
        use TypedExpressionKind as tek;
        match texpr.k {
            tek::Atom(ref t) => {
                if t.t == crate::lexer::TokenType::Identifier {
                    true
                } else {
                    false
                }
            }

            tek::Index(ref lhs, _) => self.is_targetable(&*lhs),

            tek::NamespaceProperty(ref lhs, ref rhs) => {
                self.is_targetable(&*lhs) && self.is_targetable(&*rhs)
            }

            tek::Property(ref lhs, ref rhs) => {
                self.is_targetable(&*lhs) && self.is_targetable(&*rhs)
            }

            _ => false,
        }
    }

    fn token_to_type(&self, t: &crate::lexer::Token) -> Type {
        use crate::lexer::TokenType::*;

        match t.t {
            String => Type::String,
            Integer => Type::Integer,
            Float => Type::Float,
            Character => Type::Character,
            Identifier if t.v == "true" || t.v == "false" => Type::Bool,
            Identifier if t.v == "nil" => Type::Void,
            Identifier => {
                if let Some(sym) = self.get_symbol(&t.v) {
                    sym.ty.clone()
                } else {
                    error!("Unknown Identifier Object", self.source_id, t.s, t.e);
                    Type::Unknown
                }
            }
            _ => {
                error!("Invalid Type", self.source_id, t.s, t.e);
                Type::Unknown
            }
        }
    }

    fn is_op_safe(&self, op: &crate::lexer::Token, t: &Type) -> bool {
        use crate::lexer::TokenType::*;

        if *t == Type::Integer || *t == Type::Float {
            matches!(
                op.t,
                Plus | PlusEquals
                    | Minus
                    | MinusEquals
                    | Multiply
                    | MultiplyEquals
                    | Divide
                    | DivideEquals
                    | Exponent
                    | ExponentEquals
                    | Modulous
                    | ModulousEquals
                    | Increment
                    | Decrement
                    | EqualTo
                    | NotEqualTo
            )
        } else if *t == Type::Bool {
            matches!(
                op.t,
                GreaterThan
                    | GreaterThanEqual
                    | LessThan
                    | LessThanEqual
                    | EqualTo
                    | NotEqualTo
                    | And
                    | Or
                    | Xor
                    | Not
            )
        } else if *t == Type::String {
            matches!(op.t, Plus | PlusEquals | EqualTo | NotEqualTo)
        } else {
            false
        }
    }

    fn analyse_expression(&mut self, expr: crate::parser::Expression) -> TypedExpression {
        use crate::parser::ExpressionKind::*;

        let (typed_expr_kind, typed) = match expr.k {
            Atom(a) => {
                let tt = self.token_to_type(&a);
                (TypedExpressionKind::Atom(a), tt)
            }

            Binary(lhs, op, rhs) => {
                let tlhs = self.analyse_expression(*lhs);
                let trhs = self.analyse_expression(*rhs);

                if tlhs.t != trhs.t {
                    error!("Unmatched Types", self.source_id, tlhs.s, trhs.e);
                    return TypedExpression::err();
                }

                let t = tlhs.t.clone();

                if !self.is_op_safe(&op, &t) {
                    error!(
                        "Operator Cannot Be Used On This Type",
                        self.source_id, tlhs.s, trhs.e
                    );
                    return TypedExpression::err();
                }

                (
                    TypedExpressionKind::Binary(Box::new(tlhs), op, Box::new(trhs)),
                    t,
                )
            }

            Unary(op, rhs) => {
                // not or minus
                let trhs = self.analyse_expression(*rhs);
                let t = trhs.t.clone();

                if !self.is_op_safe(&op, &t) {
                    error!(
                        "Operator Cannot Be Used On This Type",
                        self.source_id, op.s, trhs.e
                    );
                    return TypedExpression::err();
                }

                (TypedExpressionKind::Unary(op, Box::new(trhs)), t)
            }

            Array(values) => {
                if values.is_empty() {
                    (
                        TypedExpressionKind::Array(Vec::new()),
                        Type::Array(Box::new(Type::Unknown)),
                    )
                } else {
                    let mut values = values.iter();

                    let first = self.analyse_expression(values.next().unwrap().to_owned()); // assume as values isnt empty
                    let t = first.t.clone();

                    let mut typed_values = Vec::from([first]);

                    for value in values {
                        let new = self.analyse_expression(value.to_owned());

                        if new.t != t {
                            error!("Incompatable Array Types", self.source_id, value.s, value.e);
                            return TypedExpression::err();
                        }

                        typed_values.push(new);
                    }

                    (
                        TypedExpressionKind::Array(typed_values),
                        Type::Array(Box::new(t)),
                    )
                }
            }

            Call(name, params) => {
                let ns = name.s;
                let ne = name.e;

                let tname = self.analyse_expression(*name);

                let mut tparams = Vec::new();
                for param in params {
                    tparams.push(self.analyse_expression(param));
                }

                let ret = match &tname.t {
                    Type::Function(exp_p, rett) => {
                        if exp_p.len() != tparams.len() {
                            error!(
                                "Incorrect Amount Of Parameters",
                                self.source_id, expr.s, expr.e
                            );
                            return TypedExpression::err();
                        }

                        for (exp, act) in exp_p.iter().zip(&tparams) {
                            if exp != &act.t {
                                error!("Incorrect Parameter Type", self.source_id, act.s, act.e);
                                return TypedExpression::err();
                            }
                        }
                        (**rett).clone()
                    }

                    _ => {
                        error!("Callee Is Not A Function", self.source_id, ns, ne);
                        return TypedExpression::err();
                    }
                };

                (TypedExpressionKind::Call(Box::new(tname), tparams), ret)
            }

            Index(lhs, loc) => {
                let tlhs = self.analyse_expression(*lhs);
                let tloc = self.analyse_expression(*loc);

                let data_t = match &tlhs.t {
                    Type::Array(item_t) => (**item_t).clone(),

                    _ => {
                        error!("Is Not An Array", self.source_id, tlhs.s, tlhs.e);
                        return TypedExpression::err();
                    }
                };

                if tloc.t != Type::Integer {
                    error!(
                        "Array Index Must Be Integer",
                        self.source_id, tloc.s, tloc.e
                    );
                    return TypedExpression::err();
                }

                (
                    TypedExpressionKind::Index(Box::new(tlhs), Box::new(tloc)),
                    data_t,
                )
            }

            Property(lhs, rhs) => {
                let tlhs = self.analyse_expression(*lhs);
                let trhs = self.analyse_expression(*rhs);

                let t = trhs.t.clone();

                (
                    TypedExpressionKind::Property(Box::new(tlhs), Box::new(trhs)),
                    t,
                )
            }

            NamespaceProperty(lhs, rhs) => {
                let tlhs = self.analyse_expression(*lhs);
                let trhs = self.analyse_expression(*rhs);

                let t = trhs.t.clone();

                (
                    TypedExpressionKind::NamespaceProperty(Box::new(tlhs), Box::new(trhs)),
                    t,
                )
            }

            _ => {
                error!(
                    "Unknown Parser Expression Kind",
                    self.source_id, expr.s, expr.e
                );
                return TypedExpression::err();
            }
        };

        TypedExpression::new(typed, typed_expr_kind, expr.s, expr.e)
    }

    fn analyse_statement(&mut self, statement: crate::parser::Statement) -> TypedStatement {
        use crate::parser::StatementKind as Sk;

        let typed_stmt_kind = match statement.k {
            Sk::Assignment(target, value) => {
                let ttarget = self.analyse_expression(target);
                if !self.is_targetable(&ttarget) {
                    error!(
                        "Item Is Not Targetable",
                        self.source_id, ttarget.s, ttarget.e
                    )
                }

                let tvalue = self.analyse_expression(value);

                TypedStatementKind::Assignment(ttarget, tvalue)
            }

            _ => {
                error!(
                    "Unknown Statement",
                    self.source_id, statement.s, statement.e
                );
                return TypedStatement::err();
            }
        };

        TypedStatement::new(typed_stmt_kind, statement.s, statement.e)
    }

    fn analyse_scope(&mut self, stmts: Vec<crate::parser::Statement>) -> Vec<TypedStatement> {
        let mut result = Vec::<TypedStatement>::new();

        let sid = self.create_scope(None);
        self.enter_scope(sid);

        for stmt in stmts {
            let t = self.analyse_statement(stmt);
            result.push(t);
        }

        self.exit_scope();
        result
    }
}

pub fn analyse(program: Program, id: usize) -> TypedProgram {
    let mut analyser = Analyser::new(id);
    analyser.result = analyser.analyse_scope(program);
    analyser.finish()
}
