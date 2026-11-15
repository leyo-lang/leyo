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

fn ast_pretty_expression(expr: &crate::parser::Expression, prefix: &str, is_last: bool, label: &str, output: &mut String) {
    let branch = if is_last { "└── " } else { "├── " };
    let _ = writeln!(output, "{}{}{}", prefix, branch, label);

    let child_prefix = format!("{}{}", prefix, if is_last { "    " } else { "│   " });

    match expr {
        crate::parser::Expression::Error => {let _ = writeln!(output, "{}└── Error", child_prefix);}

        crate::parser::Expression::Atom(token) => {let _ = writeln!(output, "{}└── {:?}: {:?}", child_prefix, token.t, token.v);
        }

        crate::parser::Expression::Binary(lhs, op, rhs) => {
            ast_pretty_expression(
                lhs,
                &child_prefix,
                false,
                "Left",
                output,
            );

            let _ = writeln!(output, "{}├── Operator: {:?}", child_prefix, op.v);

            ast_pretty_expression(
                rhs,
                &child_prefix,
                true,
                "Right",
                output,
            );
        },

        crate::parser::Expression::Unary(op, rhs) => {
            let _ = writeln!(output, "{}├── Operator: {:?}", child_prefix, op.v);

            ast_pretty_expression(
                rhs,
                &child_prefix,
                true,
                "Right",
                output,
            );
        }
    }
}

fn ast_pretty_statement(stmt: &crate::parser::Statement, prefix: &str, is_last: bool, output: &mut String) {
    let branch = if is_last { "└── " } else { "├── " };

    let label = match stmt {
        crate::parser::Statement::None => "None".to_string(),
        crate::parser::Statement::End => "End".to_string(),
        crate::parser::Statement::Error => "Error".to_string(),
        crate::parser::Statement::Block(_) => "Block".to_string(),
        crate::parser::Statement::VariableDeclaration(name, _) => format!("VariableDeclaration: {:?}", name),
        crate::parser::Statement::ConstantDeclaration(name, _) => format!("ConstantDeclaration: {:?}", name),
        crate::parser::Statement::Assignment(name, _) => format!("Assignment: {:?}", name),
    };

    let _ = writeln!(output, "{}{}{}", prefix, branch, label);

    let child_prefix = format!("{}{}", prefix, if is_last { "    " } else { "│   " });

    match stmt {
        crate::parser::Statement::VariableDeclaration(_, expr)
        | crate::parser::Statement::ConstantDeclaration(_, expr)
        | crate::parser::Statement::Assignment(_, expr) => {
            ast_pretty_expression(
                expr,
                &child_prefix,
                true,
                "Value",
                output,
            );
        }

        crate::parser::Statement::Block(statements) => {
            for (i, stmt) in statements.iter().enumerate() {
                ast_pretty_statement(
                    stmt,
                    &child_prefix,
                    i == statements.len() - 1,
                    output,
                );
            }
        }

        _ => {}
    }
}

pub fn ast_pretty_print(program: &crate::parser::Program) -> String {
    let mut output = String::from("Program\n");

    for (i, stmt) in program.iter().enumerate() {
        ast_pretty_statement(
            stmt,
            "",
            i == program.len() - 1,
            &mut output,
        );
    }

    output
}