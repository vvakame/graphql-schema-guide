import {
  Visitor,
  ASTKindToNode,
  ValidationContext,
  GraphQLError,
  ASTNode
} from "graphql";

export class ValidationError extends GraphQLError {
  constructor(public ruleName: string, message: string, nodes: ASTNode[]) {
    // エラーに ruleName という名前でルール名を持つ必要がある
    super(message, nodes);
  }
}

export function YukariKawaii(
  context: ValidationContext
): Visitor<ASTKindToNode> {
  return {
    FieldDefinition: v => {
      if (v.name.value !== "yukari") {
        return;
      }

      context.reportError(
        new ValidationError(
          "yukari-kawaii",
          `${v.name.value}という名前は可愛すぎます！`,
          [v]
        )
      );
    }
  };
}
