use std::{fmt::Write, time::Duration};

use crate::warn;

pub struct DisplayDuration(pub Duration);
impl std::fmt::Display for DisplayDuration {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        let d = self.0;

        let secs = d.as_secs();
        let millis = d.subsec_millis();

        if secs >= 3600 {
            write!(f, "{}h {}m {}s", secs / 3600, (secs % 3600) / 60, secs % 60)
        } else if secs >= 60 {
            write!(f, "{}m {}.{:03}s", secs / 60, secs % 60, millis)
        } else if secs > 0 {
            write!(f, "{:.2}s", d.as_secs_f64())
        } else if d.as_millis() > 0 {
            write!(f, "{:.1}ms", d.as_secs_f64() * 1000.0)
        } else if d.as_micros() > 0 {
            write!(f, "{}µs", d.as_micros())
        } else {
            write!(f, "{}ns", d.as_nanos())
        }
    }
}

pub fn print_lexer_result(result: &crate::lexer::LexerResult) -> String {
    let mut output = String::new();
    for (i, token) in result.iter().enumerate() {
        let e = writeln!(
            output,
            "{:<10} {:<20} {:<20?}",
            i,
            format!("{:?}", token.t),
            token.v,
        );

        if e.is_err() {
            warn!("Could Not Write To Log")
        }
    }
    output
}

fn ast_pretty_expression(
    expr: &crate::parser::ExpressionKind,
    prefix: &str,
    is_last: bool,
    label: &str,
    output: &mut String,
) {
    let branch = if is_last { "└── " } else { "├── " };
    let _ = writeln!(output, "{}{}{}", prefix, branch, label);

    let child_prefix = format!("{}{}", prefix, if is_last { "    " } else { "│   " });

    match expr {
        crate::parser::ExpressionKind::Error => {
            let _ = writeln!(output, "{}└── Error", child_prefix);
        }

        crate::parser::ExpressionKind::Atom(token) => {
            let _ = writeln!(output, "{}└── {:?}: {:?}", child_prefix, token.t, token.v);
        }

        crate::parser::ExpressionKind::Array(array) => {
            let _ = writeln!(output, "{}└── Array", child_prefix);

            let array_prefix = format!("{}    ", child_prefix);

            for (i, element) in array.iter().enumerate() {
                let is_last_element = i == array.len() - 1;

                ast_pretty_expression(
                    &element.k,
                    &array_prefix,
                    is_last_element,
                    &format!("Element {}", i),
                    output,
                );
            }
        }

        crate::parser::ExpressionKind::Binary(lhs, op, rhs) => {
            ast_pretty_expression(&lhs.k, &child_prefix, false, "Left", output);

            let _ = writeln!(output, "{}├── Operator: {:?}", child_prefix, op.v);

            ast_pretty_expression(&rhs.k, &child_prefix, true, "Right", output);
        }

        crate::parser::ExpressionKind::Unary(op, rhs) => {
            let _ = writeln!(output, "{}├── Operator: {:?}", child_prefix, op.v);

            ast_pretty_expression(&rhs.k, &child_prefix, true, "Right", output);
        }

        crate::parser::ExpressionKind::Call(target, params) => {
            ast_pretty_expression(
                &target.k,
                &child_prefix,
                params.is_empty(),
                "Callee",
                output,
            );

            for (i, param) in params.iter().enumerate() {
                ast_pretty_expression(
                    &param.k,
                    &child_prefix,
                    i == params.len() - 1,
                    &format!("Param {i}").to_string(),
                    output,
                );
            }
        }

        crate::parser::ExpressionKind::Property(object, property) => {
            ast_pretty_expression(&object.k, &child_prefix, false, "Object", output);

            ast_pretty_expression(&property.k, &child_prefix, true, "Property", output);
        }

        crate::parser::ExpressionKind::NamespaceProperty(object, property) => {
            ast_pretty_expression(&object.k, &child_prefix, false, "Namespace", output);

            ast_pretty_expression(&property.k, &child_prefix, true, "Property", output);
        }

        crate::parser::ExpressionKind::Index(target, index) => {
            ast_pretty_expression(&target.k, &child_prefix, false, "Item", output);

            ast_pretty_expression(&index.k, &child_prefix, true, "Index", output);
        }
    }
}

fn ast_pretty_statement(
    stmt: &crate::parser::StatementKind,
    prefix: &str,
    is_last: bool,
    output: &mut String,
) {
    let branch = if is_last { "└── " } else { "├── " };

    let label = match stmt {
        crate::parser::StatementKind::None => "None".to_string(),
        crate::parser::StatementKind::End => "End".to_string(),
        crate::parser::StatementKind::Error => "Error".to_string(),
        crate::parser::StatementKind::Block(n, _, _) if n.is_none() => "Block".to_string(),
        crate::parser::StatementKind::Block(n, _, _) => {
            format!("Namespace: {}", n.as_ref().unwrap())
        }
        crate::parser::StatementKind::Loop(_, _) => "Loop".to_string(),
        crate::parser::StatementKind::If(_, _, _) => "If".to_string(),
        crate::parser::StatementKind::Repeat(_, _, _) => "Repeat".to_string(),
        crate::parser::StatementKind::VariableDeclaration(_, _, _) => {
            "VariableDeclaration".to_string()
        }
        crate::parser::StatementKind::ConstantDeclaration(_, _, _) => {
            "ConstantDeclaration".to_string()
        }
        crate::parser::StatementKind::Assignment(_, _) => "Assignment".to_string(),
        crate::parser::StatementKind::Evaluation(_) => "Expression".to_string(),
        crate::parser::StatementKind::Break => "Break".to_string(),
        crate::parser::StatementKind::Continue => "Continue".to_string(),
        crate::parser::StatementKind::Function(name, _, _, _) => format!("Function: {name}"),
        crate::parser::StatementKind::Return(_) => "Return".to_string(),
    };

    let _ = writeln!(output, "{}{}{}", prefix, branch, label);

    let child_prefix = format!("{}{}", prefix, if is_last { "    " } else { "│   " });

    match stmt {
        crate::parser::StatementKind::Evaluation(expr) => {
            ast_pretty_expression(&expr.k, &child_prefix, true, "Value", output);
        }

        crate::parser::StatementKind::VariableDeclaration(target, expr, _)
        | crate::parser::StatementKind::ConstantDeclaration(target, expr, _)
        | crate::parser::StatementKind::Assignment(target, expr) => {
            ast_pretty_expression(&target.k, &child_prefix, false, "Target", output);
            ast_pretty_expression(&expr.k, &child_prefix, true, "Value", output);
        }

        crate::parser::StatementKind::Block(_, statements, _) => {
            for (i, stmt) in statements.iter().enumerate() {
                ast_pretty_statement(&stmt.k, &child_prefix, i == statements.len() - 1, output);
            }
        }

        crate::parser::StatementKind::Loop(cond, stmt) => {
            ast_pretty_expression(&cond.k, &child_prefix, false, "Condition", output);
            ast_pretty_statement(&stmt.k, &child_prefix, true, output);
        }

        crate::parser::StatementKind::If(cond, stmt, els) => {
            ast_pretty_expression(&cond.k, &child_prefix, false, "Condition", output);
            ast_pretty_statement(&stmt.k, &child_prefix, els.is_none(), output);
            if els.is_some() {
                ast_pretty_statement(&els.as_ref().unwrap().k, &child_prefix, true, output);
            }
        }

        crate::parser::StatementKind::Repeat(cond, target, stmt) => {
            ast_pretty_expression(&cond.k, &child_prefix, false, "Count", output);
            if target.is_some() {
                ast_pretty_expression(&target.as_ref().unwrap().k, prefix, is_last, &label, output);
            };
            ast_pretty_statement(&stmt.k, &child_prefix, true, output);
        }

        crate::parser::StatementKind::Function(_, params, body, _) => {
            for (i, param) in params.iter().enumerate() {
                ast_pretty_expression(
                    &param.k,
                    &child_prefix,
                    false,
                    &format!("Param {i}").to_string(),
                    output,
                );
            }
            ast_pretty_statement(&body.k, &child_prefix, true, output);
        }

        crate::parser::StatementKind::Return(value) if value.is_some() => {
            ast_pretty_expression(
                &value.as_ref().unwrap().k,
                &child_prefix,
                true,
                "Value",
                output,
            );
        }

        _ => {}
    }
}

pub fn ast_pretty_print(program: &crate::parser::Program) -> String {
    let mut output = String::from("Program\n");

    for (i, stmt) in program.iter().enumerate() {
        ast_pretty_statement(&stmt.k, "", i == program.len() - 1, &mut output);
    }

    output
}

fn type_pretty(ty: &crate::semantic::Type) -> String {
    match ty {
        crate::semantic::Type::Integer => "integer".to_string(),
        crate::semantic::Type::Float => "float".to_string(),
        crate::semantic::Type::Bool => "bool".to_string(),
        crate::semantic::Type::String => "string".to_string(),
        crate::semantic::Type::Character => "character".to_string(),
        crate::semantic::Type::Void => "void".to_string(),
        crate::semantic::Type::Unknown => "unknown".to_string(),
        crate::semantic::Type::Array(inner) => format!("[{}]", type_pretty(inner)),
        crate::semantic::Type::Function(params, result) => {
            let params = params
                .iter()
                .map(type_pretty)
                .collect::<Vec<_>>()
                .join(", ");

            format!("fn({}) -> {}", params, type_pretty(result))
        }
    }
}

fn typed_ast_pretty_expression(
    expr: &crate::semantic::TypedExpression,
    prefix: &str,
    is_last: bool,
    label: &str,
    output: &mut String,
) {
    let branch = if is_last { "└── " } else { "├── " };
    let _ = writeln!(output, "{}{}{}", prefix, branch, label);

    let child_prefix = format!("{}{}", prefix, if is_last { "    " } else { "│   " });

    match &expr.k {
        crate::semantic::TypedExpressionKind::Error => {
            let _ = writeln!(output, "{}└── Error", child_prefix);
        }

        crate::semantic::TypedExpressionKind::Atom(token) => {
            let _ = writeln!(
                output,
                "{}└── {:?}: {:?} : {}",
                child_prefix,
                token.t,
                token.v,
                type_pretty(&expr.t)
            );
        }

        crate::semantic::TypedExpressionKind::Binary(lhs, op, rhs) => {
            let _ = writeln!(
                output,
                "{}├── Operator: {:?} : {}",
                child_prefix,
                op.v,
                type_pretty(&expr.t)
            );

            typed_ast_pretty_expression(lhs, &child_prefix, false, "Left", output);

            typed_ast_pretty_expression(rhs, &child_prefix, true, "Right", output);
        }

        crate::semantic::TypedExpressionKind::Unary(op, rhs) => {
            let _ = writeln!(
                output,
                "{}├── Operator: {:?} : {}",
                child_prefix,
                op.v,
                type_pretty(&expr.t)
            );

            typed_ast_pretty_expression(rhs, &child_prefix, true, "Right", output);
        }

        crate::semantic::TypedExpressionKind::Array(elements) => {
            let _ = writeln!(output, "{}├── Type: {}", child_prefix, type_pretty(&expr.t));

            for (i, element) in elements.iter().enumerate() {
                typed_ast_pretty_expression(
                    element,
                    &child_prefix,
                    i == elements.len() - 1,
                    &format!("Element {}", i),
                    output,
                );
            }
        }

        crate::semantic::TypedExpressionKind::Call(target, params) => {
            let _ = writeln!(
                output,
                "{}├── Return type: {}",
                child_prefix,
                type_pretty(&expr.t)
            );

            typed_ast_pretty_expression(target, &child_prefix, params.is_empty(), "Callee", output);

            for (i, param) in params.iter().enumerate() {
                typed_ast_pretty_expression(
                    param,
                    &child_prefix,
                    i == params.len() - 1,
                    &format!("Param {}", i),
                    output,
                );
            }
        }

        crate::semantic::TypedExpressionKind::Index(target, index) => {
            let _ = writeln!(output, "{}├── Type: {}", child_prefix, type_pretty(&expr.t));

            typed_ast_pretty_expression(target, &child_prefix, false, "Target", output);

            typed_ast_pretty_expression(index, &child_prefix, true, "Index", output);
        }

        crate::semantic::TypedExpressionKind::Property(object, property) => {
            let _ = writeln!(output, "{}├── Type: {}", child_prefix, type_pretty(&expr.t));

            typed_ast_pretty_expression(object, &child_prefix, false, "Object", output);

            typed_ast_pretty_expression(property, &child_prefix, true, "Property", output);
        }

        crate::semantic::TypedExpressionKind::NamespaceProperty(namespace, property) => {
            let _ = writeln!(output, "{}├── Type: {}", child_prefix, type_pretty(&expr.t));

            typed_ast_pretty_expression(namespace, &child_prefix, false, "Namespace", output);

            typed_ast_pretty_expression(property, &child_prefix, true, "Property", output);
        }
    }
}

fn typed_ast_pretty_statement(
    stmt: &crate::semantic::TypedStatement,
    prefix: &str,
    is_last: bool,
    output: &mut String,
) {
    let branch = if is_last { "└── " } else { "├── " };

    let label = match &stmt.k {
        crate::semantic::TypedStatementKind::None => "None".to_string(),
        crate::semantic::TypedStatementKind::End => "End".to_string(),
        crate::semantic::TypedStatementKind::Error => "Error".to_string(),

        crate::semantic::TypedStatementKind::Block(name, _) => match name {
            Some(name) => format!("Namespace: {}", name),
            None => "Block".to_string(),
        },

        crate::semantic::TypedStatementKind::VariableDeclaration(_, _) => {
            "VariableDeclaration".to_string()
        }

        crate::semantic::TypedStatementKind::ConstantDeclaration(_, _) => {
            "ConstantDeclaration".to_string()
        }

        crate::semantic::TypedStatementKind::Assignment(_, _) => "Assignment".to_string(),
        crate::semantic::TypedStatementKind::Evaluation(_) => "Expression".to_string(),
        crate::semantic::TypedStatementKind::Loop(_, _) => "Loop".to_string(),
        crate::semantic::TypedStatementKind::If(_, _, _) => "If".to_string(),
        crate::semantic::TypedStatementKind::Repeat(_, _, _) => "Repeat".to_string(),
        crate::semantic::TypedStatementKind::Break => "Break".to_string(),
        crate::semantic::TypedStatementKind::Continue => "Continue".to_string(),
        crate::semantic::TypedStatementKind::Function(name, _, _) => {
            format!("Function: {}", name)
        }
        crate::semantic::TypedStatementKind::Return(_) => "Return".to_string(),
    };

    let _ = writeln!(output, "{}{}{}", prefix, branch, label);

    let child_prefix = format!("{}{}", prefix, if is_last { "    " } else { "│   " });

    match &stmt.k {
        crate::semantic::TypedStatementKind::Evaluation(expr) => {
            typed_ast_pretty_expression(expr, &child_prefix, true, "Value", output);
        }

        crate::semantic::TypedStatementKind::VariableDeclaration(target, expr)
        | crate::semantic::TypedStatementKind::ConstantDeclaration(target, expr)
        | crate::semantic::TypedStatementKind::Assignment(target, expr) => {
            typed_ast_pretty_expression(target, &child_prefix, false, "Target", output);

            typed_ast_pretty_expression(expr, &child_prefix, true, "Value", output);
        }

        crate::semantic::TypedStatementKind::Block(_, statements) => {
            for (i, stmt) in statements.iter().enumerate() {
                typed_ast_pretty_statement(stmt, &child_prefix, i == statements.len() - 1, output);
            }
        }

        crate::semantic::TypedStatementKind::Loop(cond, body) => {
            typed_ast_pretty_expression(cond, &child_prefix, false, "Condition", output);

            typed_ast_pretty_statement(body, &child_prefix, true, output);
        }

        crate::semantic::TypedStatementKind::If(cond, body, els) => {
            typed_ast_pretty_expression(cond, &child_prefix, false, "Condition", output);

            typed_ast_pretty_statement(body, &child_prefix, els.is_none(), output);

            if let Some(els) = els {
                typed_ast_pretty_statement(els, &child_prefix, true, output);
            }
        }

        crate::semantic::TypedStatementKind::Repeat(count, target, body) => {
            typed_ast_pretty_expression(count, &child_prefix, target.is_none(), "Count", output);

            if let Some(target) = target {
                typed_ast_pretty_expression(target, &child_prefix, false, "Target", output);
            }

            typed_ast_pretty_statement(body, &child_prefix, true, output);
        }

        crate::semantic::TypedStatementKind::Function(_, params, body) => {
            for (i, param) in params.iter().enumerate() {
                typed_ast_pretty_expression(
                    param,
                    &child_prefix,
                    false,
                    &format!("Param {}", i),
                    output,
                );
            }

            for (i, stmt) in body.iter().enumerate() {
                typed_ast_pretty_statement(stmt, &child_prefix, i == body.len() - 1, output);
            }
        }

        crate::semantic::TypedStatementKind::Return(Some(value)) => {
            typed_ast_pretty_expression(value, &child_prefix, true, "Value", output);
        }

        _ => {}
    }
}

pub fn typed_ast_pretty_print(program: &crate::semantic::TypedProgram) -> String {
    let mut output = String::from("TypedProgram\n");

    for (i, stmt) in program.iter().enumerate() {
        typed_ast_pretty_statement(stmt, "", i == program.len() - 1, &mut output);
    }

    output
}
