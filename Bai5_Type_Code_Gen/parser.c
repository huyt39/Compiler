/* 
 * @copyright (c) 2008, Hedspi, Hanoi University of Technology
 * @author Huu-Duc Nguyen
 * @version 1.0
 */
#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include "reader.h"
#include "scanner.h"
#include "parser.h"
#include "semantics.h"
#include "error.h"
#include "debug.h"
#include "codegen.h"

Token *currentToken;
Token *lookAhead;

extern Type* intType;
extern Type* charType;
extern SymTab* symtab;

void scan(void) {
  Token* tmp = currentToken;
  currentToken = lookAhead;
  lookAhead = getValidToken();
  free(tmp);
}

void eat(TokenType tokenType) {
  if (lookAhead->tokenType == tokenType) {
    //    printToken(lookAhead);
    scan();
  } else missingToken(tokenType, lookAhead->lineNo, lookAhead->colNo);
}

void compileProgram(void) {
  Object* program;

  eat(KW_PROGRAM);
  eat(TK_IDENT);

  program = createProgramObject(currentToken->string);
  program->progAttrs->codeAddress = getCurrentCodeAddress();
  enterBlock(program->progAttrs->scope);

  eat(SB_SEMICOLON);

  compileBlock();
  eat(SB_PERIOD);

  // dung chuong trinh
  genHL();

  exitBlock();
}

void compileConstDecls(void) {
  Object* constObj;
  ConstantValue* constValue;

  if (lookAhead->tokenType == KW_CONST) {
    eat(KW_CONST);
    do {
      eat(TK_IDENT);
      checkFreshIdent(currentToken->string);
      constObj = createConstantObject(currentToken->string);
      declareObject(constObj);
      
      eat(SB_EQ);
      constValue = compileConstant();
      constObj->constAttrs->value = constValue;
      
      eat(SB_SEMICOLON);
    } while (lookAhead->tokenType == TK_IDENT);
  }
}

void compileTypeDecls(void) {
  Object* typeObj;
  Type* actualType;

  if (lookAhead->tokenType == KW_TYPE) {
    eat(KW_TYPE);
    do {
      eat(TK_IDENT);
      
      checkFreshIdent(currentToken->string);
      typeObj = createTypeObject(currentToken->string);
      declareObject(typeObj);
      
      eat(SB_EQ);
      actualType = compileType();
      typeObj->typeAttrs->actualType = actualType;
      
      eat(SB_SEMICOLON);
    } while (lookAhead->tokenType == TK_IDENT);
  } 
}

void compileVarDecls(void) {
  Object* varObj;
  Type* varType;

  if (lookAhead->tokenType == KW_VAR) {
    eat(KW_VAR);
    do {
      eat(TK_IDENT);
      checkFreshIdent(currentToken->string);
      varObj = createVariableObject(currentToken->string);
      eat(SB_COLON);
      varType = compileType();
      varObj->varAttrs->type = varType;
      declareObject(varObj);      
      eat(SB_SEMICOLON);
    } while (lookAhead->tokenType == TK_IDENT);
  } 
}

void compileBlock(void) {
  Instruction* jmp;
  // nhay den than cua block
  jmp = genJ(DC_VALUE);

  compileConstDecls();
  compileTypeDecls();
  compileVarDecls();
  compileSubDecls();

  // cap nhat nhan jmp
  updateJ(jmp,getCurrentCodeAddress());
  // bo qua stack frame
  genINT(symtab->currentScope->frameSize);

  eat(KW_BEGIN);
  compileStatements();
  eat(KW_END);
}

void compileSubDecls(void) {
  while ((lookAhead->tokenType == KW_FUNCTION) || (lookAhead->tokenType == KW_PROCEDURE)) {
    if (lookAhead->tokenType == KW_FUNCTION)
      compileFuncDecl();
    else compileProcDecl();
  }
}

void compileFuncDecl(void) {
  Object* funcObj;
  Type* returnType;

  eat(KW_FUNCTION);
  eat(TK_IDENT);

  checkFreshIdent(currentToken->string);
  funcObj = createFunctionObject(currentToken->string);
  funcObj->funcAttrs->codeAddress = getCurrentCodeAddress();
  declareObject(funcObj);

  enterBlock(funcObj->funcAttrs->scope);
  
  compileParams();

  eat(SB_COLON);
  returnType = compileBasicType();
  funcObj->funcAttrs->returnType = returnType;

  eat(SB_SEMICOLON);

  compileBlock();

  eat(SB_SEMICOLON);

  exitBlock();
}

void compileProcDecl(void) {
  Object* procObj;

  eat(KW_PROCEDURE);
  eat(TK_IDENT);

  checkFreshIdent(currentToken->string);
  procObj = createProcedureObject(currentToken->string);
  procObj->procAttrs->codeAddress = getCurrentCodeAddress();
  declareObject(procObj);

  enterBlock(procObj->procAttrs->scope);

  compileParams();

  eat(SB_SEMICOLON);
  compileBlock();

  eat(SB_SEMICOLON);

  exitBlock();
}

ConstantValue* compileUnsignedConstant(void) {
  ConstantValue* constValue;
  Object* obj;

  switch (lookAhead->tokenType) {
  case TK_NUMBER:
    eat(TK_NUMBER);
    constValue = makeIntConstant(currentToken->value);
    break;
  case TK_IDENT:
    eat(TK_IDENT);

    obj = checkDeclaredConstant(currentToken->string);
    constValue = duplicateConstantValue(obj->constAttrs->value);

    break;
  case TK_CHAR:
    eat(TK_CHAR);
    constValue = makeCharConstant(currentToken->string[0]);
    break;
  default:
    error(ERR_INVALID_CONSTANT, lookAhead->lineNo, lookAhead->colNo);
    break;
  }
  return constValue;
}

ConstantValue* compileConstant(void) {
  ConstantValue* constValue;

  switch (lookAhead->tokenType) {
  case SB_PLUS:
    eat(SB_PLUS);
    constValue = compileConstant2();
    break;
  case SB_MINUS:
    eat(SB_MINUS);
    constValue = compileConstant2();
    constValue->intValue = - constValue->intValue;
    break;
  case TK_CHAR:
    eat(TK_CHAR);
    constValue = makeCharConstant(currentToken->string[0]);
    break;
  default:
    constValue = compileConstant2();
    break;
  }
  return constValue;
}

ConstantValue* compileConstant2(void) {
  ConstantValue* constValue;
  Object* obj;

  switch (lookAhead->tokenType) {
  case TK_NUMBER:
    eat(TK_NUMBER);
    constValue = makeIntConstant(currentToken->value);
    break;
  case TK_IDENT:
    eat(TK_IDENT);
    obj = checkDeclaredConstant(currentToken->string);
    if (obj->constAttrs->value->type == TP_INT)
      constValue = duplicateConstantValue(obj->constAttrs->value);
    else
      error(ERR_UNDECLARED_INT_CONSTANT,currentToken->lineNo, currentToken->colNo);
    break;
  default:
    error(ERR_INVALID_CONSTANT, lookAhead->lineNo, lookAhead->colNo);
    break;
  }
  return constValue;
}

Type* compileType(void) {
  Type* type;
  Type* elementType;
  int arraySize;
  Object* obj;

  switch (lookAhead->tokenType) {
  case KW_INTEGER: 
    eat(KW_INTEGER);
    type =  makeIntType();
    break;
  case KW_CHAR: 
    eat(KW_CHAR); 
    type = makeCharType();
    break;
  case KW_ARRAY:
    eat(KW_ARRAY);
    eat(SB_LSEL);
    eat(TK_NUMBER);

    arraySize = currentToken->value;

    eat(SB_RSEL);
    eat(KW_OF);
    elementType = compileType();
    type = makeArrayType(arraySize, elementType);
    break;
  case TK_IDENT:
    eat(TK_IDENT);
    obj = checkDeclaredType(currentToken->string);
    type = duplicateType(obj->typeAttrs->actualType);
    break;
  default:
    error(ERR_INVALID_TYPE, lookAhead->lineNo, lookAhead->colNo);
    break;
  }
  return type;
}

Type* compileBasicType(void) {
  Type* type;

  switch (lookAhead->tokenType) {
  case KW_INTEGER: 
    eat(KW_INTEGER); 
    type = makeIntType();
    break;
  case KW_CHAR: 
    eat(KW_CHAR); 
    type = makeCharType();
    break;
  default:
    error(ERR_INVALID_BASICTYPE, lookAhead->lineNo, lookAhead->colNo);
    break;
  }
  return type;
}

void compileParams(void) {
  if (lookAhead->tokenType == SB_LPAR) {
    eat(SB_LPAR);
    compileParam();
    while (lookAhead->tokenType == SB_SEMICOLON) {
      eat(SB_SEMICOLON);
      compileParam();
    }
    eat(SB_RPAR);
  }
}

void compileParam(void) {
  Object* param;
  Type* type;
  enum ParamKind paramKind = PARAM_VALUE;

  if (lookAhead->tokenType == KW_VAR) {
    paramKind = PARAM_REFERENCE;
    eat(KW_VAR);
  }

  eat(TK_IDENT);
  checkFreshIdent(currentToken->string);
  param = createParameterObject(currentToken->string, paramKind);
  eat(SB_COLON);
  type = compileBasicType();
  param->paramAttrs->type = type;
  declareObject(param);
}

void compileStatements(void) {
  compileStatement();
  while (lookAhead->tokenType == SB_SEMICOLON) {
    eat(SB_SEMICOLON);
    compileStatement();
  }
}

void compileStatement(void) {
  switch (lookAhead->tokenType) {
  case TK_IDENT:
    compileAssignSt();
    break;
  case KW_CALL:
    compileCallSt();
    break;
  case KW_BEGIN:
    compileGroupSt();
    break;
  case KW_IF:
    compileIfSt();
    break;
  case KW_WHILE:
    compileWhileSt();
    break;
  case KW_FOR:
    compileForSt();
    break;
    // EmptySt canh bao FOLLOW tokens
  case SB_SEMICOLON:
  case KW_END:
  case KW_ELSE:
    break;
    // loi xay ra
  default:
    error(ERR_INVALID_STATEMENT, lookAhead->lineNo, lookAhead->colNo);
    break;
  }
}

Type* compileLValue(void) {
  Object* var;
  Type* varType;

  eat(TK_IDENT);
  
  var = checkDeclaredLValueIdent(currentToken->string);

  switch (var->kind) {
  case OBJ_VARIABLE:
    // day dia chi bien len stack
    genVariableAddress(var);
    if (var->varAttrs->type->typeClass == TP_ARRAY) {
      // tinh dia chi phan tu
      varType = compileIndexes(var->varAttrs->type);
    }
    else
      varType = var->varAttrs->type;
    break;
  case OBJ_PARAMETER:
    // tam thoi: dung chuong trinh
    genHL();
    varType = var->paramAttrs->type;
    break;
  case OBJ_FUNCTION:
    // tam thoi: dung chuong trinh
    genHL();
    varType = var->funcAttrs->returnType;
    break;
  default: 
    error(ERR_INVALID_LVALUE,currentToken->lineNo, currentToken->colNo);
  }

  return varType;
}

void compileAssignSt(void) {
  // sinh ma cho phep gan
  Type* varType;
  Type* expType;

  varType = compileLValue();
  
  eat(SB_ASSIGN);
  expType = compileExpression();
  checkTypeEquality(varType, expType);
  genST();
}

void compileCallSt(void) {
  // sinh ma cho cau lenh call
  // tam thoi: dung
  Object* proc;

  eat(KW_CALL);
  eat(TK_IDENT);

  proc = checkDeclaredProcedure(currentToken->string);

  if (isPredefinedProcedure(proc)) {
    compileArguments(proc->procAttrs->paramList);
    genPredefinedProcedureCall(proc);
  } else {
    compileArguments(proc->procAttrs->paramList);
    genHL();
  }
}

void compileGroupSt(void) {
  eat(KW_BEGIN);
  compileStatements();
  eat(KW_END);
}

void compileIfSt(void) {
  // sinh ma cho cau lenh if
  Instruction* fjInst;
  Instruction* jInst;

  eat(KW_IF);
  compileCondition();
  eat(KW_THEN);
  
  fjInst = genFJ(DC_VALUE);
  compileStatement();
  
  if (lookAhead->tokenType == KW_ELSE) {
    jInst = genJ(DC_VALUE);
    updateFJ(fjInst, getCurrentCodeAddress());
    eat(KW_ELSE);
    compileStatement();
    updateJ(jInst, getCurrentCodeAddress());
  } else {
    updateFJ(fjInst, getCurrentCodeAddress());
  }
}

void compileWhileSt(void) {
  // sinh ma cho cau lenh while
  CodeAddress loopStart;
  Instruction* fjInst;

  loopStart = getCurrentCodeAddress();
  eat(KW_WHILE);
  compileCondition();
  fjInst = genFJ(DC_VALUE);
  eat(KW_DO);
  compileStatement();
  genJ(loopStart);
  updateFJ(fjInst, getCurrentCodeAddress());
}

void compileForSt(void) {
  // sinh ma cho cau lenh for
  Type* varType;
  Type* exprType;
  CodeAddress loopStart;
  Instruction* fjInst;
  Object* loopVar;

  eat(KW_FOR);

  // phan tich va luu thong tin bien lap
  eat(TK_IDENT);
  loopVar = checkDeclaredLValueIdent(currentToken->string);
  varType = loopVar->varAttrs->type;

  // khoi tao: var := start
  genVariableAddress(loopVar);  // day dia chi bien lap
  eat(SB_ASSIGN);
  exprType = compileExpression();  // tinh gia tri bat dau
  checkTypeEquality(varType, exprType);
  genST();  // var := start

  // bat dau vong lap
  loopStart = getCurrentCodeAddress();
  
  // kiem tra dieu kien: var <= end
  genVariableValue(loopVar);  // nap gia tri bien
  eat(KW_TO);
  exprType = compileExpression();  // tinh gia tri ket thuc
  checkTypeEquality(varType, exprType);
  genLE();  // kiem tra var <= end
  fjInst = genFJ(DC_VALUE);  // neu sai, thoat vong lap

  // thuc thi than vong lap
  eat(KW_DO);
  compileStatement();

  // tang: var := var + 1
  genVariableAddress(loopVar);  // day dia chi var
  genVariableValue(loopVar);  // nap gia tri hien tai
  genLC(1);  // day 1
  genAD();  // cong
  genST();  // luu lai vao var
  
  genJ(loopStart);  // nhay ve dau vong lap
  updateFJ(fjInst, getCurrentCodeAddress());
}

void compileArgument(Object* param) {
  // phan tich mot doi so va kiem tra tinh nhat quan kieu
  // neu tham so tuong ung la tham chieu, doi so phai la lvalue
  Type* argType;

  if (param->paramAttrs->kind == PARAM_VALUE) {
    argType = compileExpression();
    checkTypeEquality(argType, param->paramAttrs->type);
  } else {
    // tham so tham chieu: chi chap nhan lvalue
    // kiem tra xem co phai bieu thuc khong (co toan tu sau identifier)
    if (lookAhead->tokenType == TK_IDENT) {
      // luu vi tri va ten
      int savedLine = lookAhead->lineNo;
      int savedCol = lookAhead->colNo;
      char savedName[MAX_IDENT_LEN + 1];
      strcpy(savedName, lookAhead->string);
      
      // peek token tiep theo
      eat(TK_IDENT);
      
      // kiem tra xem co toan tu ngay sau identifier khong
      if (lookAhead->tokenType == SB_PLUS || 
          lookAhead->tokenType == SB_MINUS ||
          lookAhead->tokenType == SB_TIMES ||
          lookAhead->tokenType == SB_SLASH ||
          lookAhead->tokenType == SB_LPAR) {
        // la bieu thuc, khong the truyen cho tham so tham chieu
        error(ERR_TYPE_INCONSISTENCY, savedLine, savedCol);
        // parse bieu thuc de tiep tuc
        // bat dau tu + (vi da eat identifier roi)
        argType = compileExpression();
      } else {
        // la lvalue hop le, xu ly truc tiep
        Object* var = checkDeclaredLValueIdent(savedName);
        switch (var->kind) {
        case OBJ_VARIABLE:
          genVariableAddress(var);
          if (var->varAttrs->type->typeClass == TP_ARRAY) {
            argType = compileIndexes(var->varAttrs->type);
          } else {
            argType = var->varAttrs->type;
          }
          break;
        case OBJ_PARAMETER:
          genHL();
          argType = var->paramAttrs->type;
          break;
        case OBJ_FUNCTION:
          genHL();
          argType = var->funcAttrs->returnType;
          break;
        default:
          error(ERR_INVALID_LVALUE, savedLine, savedCol);
          argType = NULL;
          break;
        }
        checkTypeEquality(argType, param->paramAttrs->type);
      }
    } else {
      // khong phai identifier, khong the la lvalue
      error(ERR_TYPE_INCONSISTENCY, lookAhead->lineNo, lookAhead->colNo);
      argType = compileExpression();
    }
  }
}

void compileArguments(ObjectNode* paramList) {
  // phan tich danh sach doi so, kiem tra tinh nhat quan giua doi so va tham so
  ObjectNode* paramNode = paramList;

  switch (lookAhead->tokenType) {
  case SB_LPAR:
    eat(SB_LPAR);
    if (paramNode == NULL)
      error(ERR_PARAMETERS_ARGUMENTS_INCONSISTENCY, currentToken->lineNo, currentToken->colNo);
    compileArgument(paramNode->object);
    paramNode = paramNode->next;

    while (lookAhead->tokenType == SB_COMMA) {
      eat(SB_COMMA);
      if (paramNode == NULL)
	error(ERR_PARAMETERS_ARGUMENTS_INCONSISTENCY, currentToken->lineNo, currentToken->colNo);
      compileArgument(paramNode->object);
      paramNode = paramNode->next;
    }

    if (paramNode != NULL)
      error(ERR_PARAMETERS_ARGUMENTS_INCONSISTENCY, currentToken->lineNo, currentToken->colNo);
    
    eat(SB_RPAR);
    break;
    // kiem tra tap FOLLOW 
  case SB_TIMES:
  case SB_SLASH:
  case SB_PLUS:
  case SB_MINUS:
  case KW_TO:
  case KW_DO:
  case SB_RPAR:
  case SB_COMMA:
  case SB_EQ:
  case SB_NEQ:
  case SB_LE:
  case SB_LT:
  case SB_GE:
  case SB_GT:
  case SB_RSEL:
  case SB_SEMICOLON:
  case KW_END:
  case KW_ELSE:
  case KW_THEN:
  	if (paramNode != NULL)
      error(ERR_PARAMETERS_ARGUMENTS_INCONSISTENCY, currentToken->lineNo, currentToken->colNo);
    break;
    
  default:
    error(ERR_INVALID_ARGUMENTS, lookAhead->lineNo, lookAhead->colNo);
  }
}


void compileCondition(void) {
  // sinh ma cho dieu kien
  Type* type1;
  Type* type2;
  TokenType op;

  type1 = compileExpression();
  checkBasicType(type1);

  op = lookAhead->tokenType;
  switch (op) {
  case SB_EQ:
    eat(SB_EQ);
    break;
  case SB_NEQ:
    eat(SB_NEQ);
    break;
  case SB_LE:
    eat(SB_LE);
    break;
  case SB_LT:
    eat(SB_LT);
    break;
  case SB_GE:
    eat(SB_GE);
    break;
  case SB_GT:
    eat(SB_GT);
    break;
  default:
    error(ERR_INVALID_COMPARATOR, lookAhead->lineNo, lookAhead->colNo);
  }

  type2 = compileExpression();
  checkTypeEquality(type1,type2);
  
  switch (op) {
  case SB_EQ:
    genEQ();
    break;
  case SB_NEQ:
    genNE();
    break;
  case SB_LE:
    genLE();
    break;
  case SB_LT:
    genLT();
    break;
  case SB_GE:
    genGE();
    break;
  case SB_GT:
    genGT();
    break;
  default:
    break;
  }
}

Type* compileExpression(void) {
  // sinh ma cho bieu thuc
  Type* type;
  
  switch (lookAhead->tokenType) {
  case SB_PLUS:
    eat(SB_PLUS);
    type = compileExpression2();
    checkIntType(type);
    break;
  case SB_MINUS:
    eat(SB_MINUS);
    type = compileExpression2();
    genNEG();
    checkIntType(type);
    break;
  default:
    type = compileExpression2();
  }
  return type;
}

Type* compileExpression2(void) {
  Type* type;

  type = compileTerm();
  type = compileExpression3(type);

  return type;
}


Type* compileExpression3(Type* argType1) {
  // sinh ma cho expression3
  Type* argType2;
  Type* resultType;

  switch (lookAhead->tokenType) {
  case SB_PLUS:
    eat(SB_PLUS);
    checkIntType(argType1);
    argType2 = compileTerm();
    genAD();
    checkIntType(argType2);

    resultType = compileExpression3(argType1);
    break;
  case SB_MINUS:
    eat(SB_MINUS);
    checkIntType(argType1);
    argType2 = compileTerm();
    genSB();
    checkIntType(argType2);

    resultType = compileExpression3(argType1);
    break;
    // kiem tra tap FOLLOW
  case KW_TO:
  case KW_DO:
  case SB_RPAR:
  case SB_COMMA:
  case SB_EQ:
  case SB_NEQ:
  case SB_LE:
  case SB_LT:
  case SB_GE:
  case SB_GT:
  case SB_RSEL:
  case SB_SEMICOLON:
  case KW_END:
  case KW_ELSE:
  case KW_THEN:
    resultType = argType1;
    break;
  default:
    error(ERR_INVALID_EXPRESSION, lookAhead->lineNo, lookAhead->colNo);
  }
  return resultType;
}

Type* compileTerm(void) {
  Type* type;
  type = compileFactor();
  type = compileTerm2(type);

  return type;
}

Type* compileTerm2(Type* argType1) {
  // sinh ma cho term2
  Type* argType2;
  Type* resultType;

  switch (lookAhead->tokenType) {
  case SB_TIMES:
    eat(SB_TIMES);
    checkIntType(argType1);
    argType2 = compileFactor();
    checkIntType(argType2);
    genML();

    resultType = compileTerm2(argType1);
    break;
  case SB_SLASH:
    eat(SB_SLASH);
    checkIntType(argType1);
    argType2 = compileFactor();
    checkIntType(argType2);
    genDV();

    resultType = compileTerm2(argType1);
    break;
    // kiem tra tap FOLLOW
  case SB_PLUS:
  case SB_MINUS:
  case KW_TO:
  case KW_DO:
  case SB_RPAR:
  case SB_COMMA:
  case SB_EQ:
  case SB_NEQ:
  case SB_LE:
  case SB_LT:
  case SB_GE:
  case SB_GT:
  case SB_RSEL:
  case SB_SEMICOLON:
  case KW_END:
  case KW_ELSE:
  case KW_THEN:
    resultType = argType1;
    break;
  default:
    error(ERR_INVALID_TERM, lookAhead->lineNo, lookAhead->colNo);
  }
  return resultType;
}

Type* compileFactor(void) {
  // sinh ma cho factor
  Type* type;
  Object* obj;

  switch (lookAhead->tokenType) {
  case TK_NUMBER:
    eat(TK_NUMBER);
    genLC(currentToken->value);
    type = intType;
    break;
  case TK_CHAR:
    eat(TK_CHAR);
    genLC(currentToken->string[0]);
    type = charType;
    break;
  case TK_IDENT:
    eat(TK_IDENT);
    obj = checkDeclaredIdent(currentToken->string);

    switch (obj->kind) {
    case OBJ_CONSTANT:
      switch (obj->constAttrs->value->type) {
      case TP_INT:
	genLC(obj->constAttrs->value->intValue);
	type = intType;
	break;
      case TP_CHAR:
	genLC(obj->constAttrs->value->charValue);
	type = charType;
	break;
      default:
	break;
      }
      break;
    case OBJ_VARIABLE:
      if (obj->varAttrs->type->typeClass == TP_ARRAY) {
	// tam thoi: dung
	type = compileIndexes(obj->varAttrs->type);
	genHL();
      } else {
	genVariableValue(obj);
	type = obj->varAttrs->type;
      }
      break;
    case OBJ_PARAMETER:
      // tam thoi: dung
      type = obj->paramAttrs->type;
      genHL();
      break;
    case OBJ_FUNCTION:
      if (isPredefinedFunction(obj)) {
	compileArguments(obj->funcAttrs->paramList);
	genPredefinedFunctionCall(obj);
      } else {
	// tam thoi: dung
	compileArguments(obj->funcAttrs->paramList);
	genHL();
      }
      type = obj->funcAttrs->returnType;
      break;
    default: 
      error(ERR_INVALID_FACTOR,currentToken->lineNo, currentToken->colNo);
      break;
    }
    break;
  case SB_LPAR:
    eat(SB_LPAR);
    type = compileExpression();
    eat(SB_RPAR);
    break;
  default:
    error(ERR_INVALID_FACTOR, lookAhead->lineNo, lookAhead->colNo);
  }
  
  return type;
}

Type* compileIndexes(Type* arrayType) {
  // tam thoi: dung
  Type* type;

  
  while (lookAhead->tokenType == SB_LSEL) {
    eat(SB_LSEL);
    type = compileExpression();
    checkIntType(type);
    checkArrayType(arrayType);

    genHL();

    arrayType = arrayType->elementType;
    eat(SB_RSEL);
  }
  checkBasicType(arrayType);
  return arrayType;
}

int compile(char *fileName) {
  if (openInputStream(fileName) == IO_ERROR)
    return IO_ERROR;

  currentToken = NULL;
  lookAhead = getValidToken();

  initSymTab();

  compileProgram();

  cleanSymTab();
  free(currentToken);
  free(lookAhead);
  closeInputStream();
  return IO_SUCCESS;

}
