use std::fmt::Write;

pub fn print_lexer_result(result: &crate::lexer::LexerResult) {
    for (i, token) in result.iter().enumerate() {
        println!(
            "{:<10} {:<20} {:<20?}",
            i,
            format!("{:?}", token.t),
            token.v
        );
    }
}

fn ast_pretty_target(
    target: &crate::parser::Target,
    prefix: &str,
    is_last: bool,
    label: &str,
    output: &mut String,
) {
    let branch = if is_last { "└── " } else { "├── " };
    let _ = writeln!(output, "{}{}{}", prefix, branch, label);

    let child_prefix = format!("{}{}", prefix, if is_last { "    " } else { "│   " });

    match target {
        crate::parser::Target::Error => {
            let _ = writeln!(output, "{}└── Error", child_prefix);
        }

        crate::parser::Target::Object(name) => {
            let _ = writeln!(output, "{}└── Object: {}", child_prefix, name);
        }

        crate::parser::Target::Property(parent, property) => {
            let _ = writeln!(output, "{}├── Property: {}", child_prefix, property);

            ast_pretty_target(parent, &child_prefix, true, "Parent", output);
        }

        crate::parser::Target::Index(parent, index) => {
            ast_pretty_target(parent, &child_prefix, false, "Target", output);

            ast_pretty_expression(index, &child_prefix, true, "Index", output);
        }
    }
}

fn ast_pretty_expression(
    expr: &crate::parser::Expression,
    prefix: &str,
    is_last: bool,
    label: &str,
    output: &mut String,
) {
    let branch = if is_last { "└── " } else { "├── " };
    let _ = writeln!(output, "{}{}{}", prefix, branch, label);

    let child_prefix = format!("{}{}", prefix, if is_last { "    " } else { "│   " });

    match expr {
        crate::parser::Expression::Error => {
            let _ = writeln!(output, "{}└── Error", child_prefix);
        }

        crate::parser::Expression::Atom(token) => {
            let _ = writeln!(output, "{}└── {:?}: {:?}", child_prefix, token.t, token.v);
        }

        crate::parser::Expression::Array(array) => {
            let _ = writeln!(output, "{}└── Array", child_prefix);

            let array_prefix = format!("{}    ", child_prefix);

            for (i, element) in array.iter().enumerate() {
                let is_last_element = i == array.len() - 1;

                ast_pretty_expression(
                    element,
                    &array_prefix,
                    is_last_element,
                    &format!("Element {}", i),
                    output,
                );
            }
        }

        crate::parser::Expression::Binary(lhs, op, rhs) => {
            ast_pretty_expression(lhs, &child_prefix, false, "Left", output);

            let _ = writeln!(output, "{}├── Operator: {:?}", child_prefix, op.v);

            ast_pretty_expression(rhs, &child_prefix, true, "Right", output);
        }

        crate::parser::Expression::Unary(op, rhs) => {
            let _ = writeln!(output, "{}├── Operator: {:?}", child_prefix, op.v);

            ast_pretty_expression(rhs, &child_prefix, true, "Right", output);
        }

        crate::parser::Expression::Call(target, params) => {
            ast_pretty_expression(
                target,
                &child_prefix,
                if params.is_empty() { true } else { false },
                "Callee",
                output,
            );

            for (i, param) in params.iter().enumerate() {
                ast_pretty_expression(
                    param,
                    &child_prefix,
                    if i == params.len() - 1 { true } else { false },
                    &format!("Param {i}").to_string(),
                    output,
                );
            }
        }

        crate::parser::Expression::Index(target, index) => {
            ast_pretty_expression(target, &child_prefix, false, "Item", output);

            ast_pretty_expression(index, &child_prefix, true, "Index", output);
        }
    }
}

fn ast_pretty_statement(
    stmt: &crate::parser::Statement,
    prefix: &str,
    is_last: bool,
    output: &mut String,
) {
    let branch = if is_last { "└── " } else { "├── " };

    let label = match stmt {
        crate::parser::Statement::None => "None".to_string(),
        crate::parser::Statement::End => "End".to_string(),
        crate::parser::Statement::Error => "Error".to_string(),
        crate::parser::Statement::Block(_) => "Block".to_string(),
        crate::parser::Statement::VariableDeclaration(_, _) => "VariableDeclaration".to_string(),
        crate::parser::Statement::ConstantDeclaration(_, _) => "ConstantDeclaration".to_string(),
        crate::parser::Statement::Assignment(_, _) => "Assignment".to_string(),
        crate::parser::Statement::Expression(_) => "Expression".to_string(),
    };

    let _ = writeln!(output, "{}{}{}", prefix, branch, label);

    let child_prefix = format!("{}{}", prefix, if is_last { "    " } else { "│   " });

    match stmt {
        crate::parser::Statement::Expression(expr) => {
            ast_pretty_expression(expr, &child_prefix, true, "Value", output);
        }

        crate::parser::Statement::VariableDeclaration(target, expr)
        | crate::parser::Statement::ConstantDeclaration(target, expr)
        | crate::parser::Statement::Assignment(target, expr) => {
            ast_pretty_target(target, &child_prefix, false, "Target", output);
            ast_pretty_expression(expr, &child_prefix, true, "Value", output);
        }

        crate::parser::Statement::Block(statements) => {
            for (i, stmt) in statements.iter().enumerate() {
                ast_pretty_statement(stmt, &child_prefix, i == statements.len() - 1, output);
            }
        }

        _ => {}
    }
}

pub fn ast_pretty_print(program: &crate::parser::Program) -> String {
    let mut output = String::from("Program\n");

    for (i, stmt) in program.iter().enumerate() {
        ast_pretty_statement(stmt, "", i == program.len() - 1, &mut output);
    }

    output
}
