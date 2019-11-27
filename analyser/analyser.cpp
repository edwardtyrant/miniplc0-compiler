#include "analyser.h"

#include <climits>
#include <sstream>

namespace miniplc0 {
	std::pair<std::vector<Instruction>, std::optional<CompilationError>> Analyser::Analyse() {
		auto err = analyseProgram();
		if (err.has_value())
			return std::make_pair(std::vector<Instruction>(), err);
		else
			return std::make_pair(_instructions, std::optional<CompilationError>());
	}

	// <程序> ::= 'begin'<主过程>'end'
	std::optional<CompilationError> Analyser::analyseProgram() {
		// 示例函数，示例如何调用子程序

		// 'begin'
		auto bg = nextToken();
		if (!bg.has_value() || bg.value().GetType() != TokenType::BEGIN)
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoBegin);

		// <主过程>
		auto err = analyseMain();
		if (err.has_value())
			return err;

		// 'end'
		auto ed = nextToken();
		if (!ed.has_value() || ed.value().GetType() != TokenType::END)
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoEnd);
		return {};
	}

	// <主过程> ::= <常量声明><变量声明><语句序列>
	// 需要补全 已补全
	std::optional<CompilationError> Analyser::analyseMain() {
		// 完全可以参照 <程序> 编写

		// <常量声明>
		auto constantDeclarationError = analyseConstantDeclaration();
		if(constantDeclarationError.has_value())
		    return constantDeclarationError;

		// <变量声明>
        auto variableDeclarationError = analyseVariableDeclaration();
        if(variableDeclarationError.has_value())
            return variableDeclarationError;

		// <语句序列>
        auto statementSequenceError = analyseStatementSequence();
        if(statementSequenceError.has_value())
            return statementSequenceError;

		// 没有错误 返回空值
		return {};
	}

	// <常量声明> ::= {<常量声明语句>}
	// <常量声明语句> ::= 'const'<标识符>'='<常表达式>';'
	std::optional<CompilationError> Analyser::analyseConstantDeclaration() {
		// 示例函数，示例如何分析常量声明

		// 常量声明语句可能有 0 或无数个
		while (true) {

		    // 1. 首先处理声明时候停止（后面return其实也是停止）
		    // 下面两种情况都是不再继续增加常量声明语句了，由{}得出可以使用0-无穷个常量声明语句
		    // 情况一：没有token了
			// 预读一个 token，不然不知道是否应该用 <常量声明> 推导
			auto next = nextToken();
			if (!next.has_value())
				return {};
			// 情况二：token不是const
            // 如果是 const 那么说明应该推导 <常量声明> 否则直接返回
			if (next.value().GetType() != TokenType::CONST) {
				unreadToken();
				return {};
			}

            // 2. 判断常量声明语句是否合理
			// <常量声明语句>
			// 前面已经判断了const了这里从标识符开始
			// 标识符
			next = nextToken();
			if (!next.has_value() || next.value().GetType() != TokenType::IDENTIFIER)
				return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNeedIdentifier);
			if (isDeclared(next.value().GetValueString()))
				return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrDuplicateDeclaration);
			addConstant(next.value());

			// '='
			next = nextToken();
			if (!next.has_value() || next.value().GetType() != TokenType::EQUAL_SIGN)
				return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrConstantNeedValue);

			// <常表达式>
			int32_t val;
			auto err = analyseConstantExpression(val);
			if (err.has_value())
				return err;

			// ';'
			next = nextToken();
			if (!next.has_value() || next.value().GetType() != TokenType::SEMICOLON)
				return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoSemicolon);
			// 生成一次 LIT 指令加载常量
			_instructions.emplace_back(Operation::LIT, val);
		}
		return {};
	};;

	// <变量声明> ::= {<变量声明语句>}
	// <变量声明语句> ::= 'var'<标识符>['='<表达式>]';'
	// 需要补全 已补全
	std::optional<CompilationError> Analyser::analyseVariableDeclaration() {
		// 变量声明语句可能有一个或者多个
        while(true){

            // 1. 首先处理声明时候停止（后面return其实也是停止）
            // 下面两种情况都是不再继续增加变量声明语句了，由{}得出可以使用0-无穷个变量声明语句

            // 预读？
            // 情况一：没有token了
            // 预读一个 token，不然不知道是否应该用 <变量声明> 推导
            auto next = nextToken();
            if (!next.has_value())
                return {};

            // 'var'
            // 情况二：token不是const
            // 如果是 const 那么说明应该推导 <变量声明> 否则直接返回
            if (next.value().GetType() != TokenType::VAR) {
                unreadToken();
                return {};
            }

            // 2. 判断变量声明语句是否合理
            // 前面已经判断了var了这里从标识符开始
            // <标识符>
            next = nextToken();
            if (!next.has_value() || next.value().GetType() != TokenType::IDENTIFIER)
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNeedIdentifier);
            if (isDeclared(next.value().GetValueString()))
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrDuplicateDeclaration);

            // 变量可能没有初始化，仍然需要一次预读
            // 预读？
            // 对是否出现错误进行判断 不初始化 要有';' 初始化要有 '='

            auto tmp = next;
            next = nextToken();
            if (!next.has_value())
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoSemicolon);
            else if (next.value().GetType() == TokenType::SEMICOLON){
            // 情况一：不初始化,保存初始化变量
                addUninitializedVariable(tmp.value());
                _instructions.emplace_back(Operation::LIT, 0);
                continue;
            }else if (next.value().GetType() != TokenType::EQUAL_SIGN){
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidVariableDeclaration);
            }


            // 情况二：初始化
            // 经过上面两个判断，执行到这里next的type必定是EQUAL_SIGN

            // '<表达式>'
            auto err = analyseExpression();
            if (err.has_value())
                return err;

            // ';'
            next = nextToken();
            if (!next.has_value() || next.value().GetType() != TokenType::SEMICOLON)
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoSemicolon);

            // 语法正确，添加变量到map中去
            addVariable(tmp.value());

        }
		return {};
	}

	// <语句序列> ::= {<语句>}
	// <语句> :: = <赋值语句> | <输出语句> | <空语句>
	// <赋值语句> :: = <标识符>'='<表达式>';'
	// <输出语句> :: = 'print' '(' <表达式> ')' ';'
	// <空语句> :: = ';'
	// 需要补全 已补全
	std::optional<CompilationError> Analyser::analyseStatementSequence() {
		while (true) {
			// 预读
			auto next = nextToken();

			// 不满足语句序列语法的几种开头情况 返回{}
			if (!next.has_value())
				return {};
			if (next.value().GetType() != TokenType::IDENTIFIER &&
				next.value().GetType() != TokenType::PRINT &&
				next.value().GetType() != TokenType::SEMICOLON) {
                unreadToken();
				return {};
			}

			// 匹配了语句序列开头语法后
			std::optional<CompilationError> err;
			switch (next.value().GetType()) {
				// 这里需要你针对不同的预读结果来调用不同的子程序
				// 注意我们没有针对空语句单独声明一个函数，因此可以直接在这里返回
			    case TokenType::IDENTIFIER:{
                    unreadToken();
                    err = analyseAssignmentStatement();
                    if (err.has_value())
                        return err;
                    break;
			    }
			    case TokenType::PRINT:{
                    unreadToken();
                    err = analyseOutputStatement();
                    if (err.has_value())
                        return err;
                    break;
                }
                case TokenType::SEMICOLON:{
                    break;
                }
                default:
                    break;
			}
		}
		return {};
	}

	// <常表达式> ::= [<符号>]<无符号整数>
	// 需要补全  已补全
	std::optional<CompilationError> Analyser::analyseConstantExpression(int32_t& out) {
		// out 是常表达式的结果
		// 这里你要分析常表达式并且计算结果
		// 注意以下均为常表达式
		// +1 -1 1
		// 同时要注意是否溢出

        auto next = nextToken();
        if (!next.has_value())
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrAssignToConstant);

        std::stringstream ss;
        // 判断是+1 还是 -1 还是 1
        if(next.value().GetType() == TokenType::PLUS_SIGN || next.value().GetType() == TokenType::MINUS_SIGN){
            ss << next.value().GetValueString();
            next = nextToken();
        }else if(next.value().GetType() == TokenType::UNSIGNED_INTEGER){
            // 占位 不做任何事
            int a =0;
        }else{
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrAssignToConstant);
        }

        if(next.value().GetType() == TokenType::UNSIGNED_INTEGER){
            ss << next.value().GetValueString();
            ss >> out;
        }else{
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrAssignToConstant);
        }

		return {};
	}

	// <表达式> ::= <项>{<加法型运算符><项>}
	std::optional<CompilationError> Analyser::analyseExpression() {
		// <项>
		auto err = analyseItem();
		if (err.has_value())
			return err;

		// {<加法型运算符><项>}
		while (true) {
			// 预读
			auto next = nextToken();
			if (!next.has_value())
				return {};
			auto type = next.value().GetType();
			if (type != TokenType::PLUS_SIGN && type != TokenType::MINUS_SIGN) {
				unreadToken();
				return {};
			}

			// <项>
			err = analyseItem();
			if (err.has_value())
				return err;

			// 根据结果生成指令
			if (type == TokenType::PLUS_SIGN)
				_instructions.emplace_back(Operation::ADD, 0);
			else if (type == TokenType::MINUS_SIGN)
				_instructions.emplace_back(Operation::SUB, 0);
		}
		return {};
	}

	// <赋值语句> ::= <标识符>'='<表达式>';'
	// 需要补全 已补全
	std::optional<CompilationError> Analyser::analyseAssignmentStatement() {
		// 这里除了语法分析以外还要留意
        auto next = nextToken();

        // 判断是否开头为标识符
		if(!next.has_value() || next.value().GetType() != TokenType::IDENTIFIER){
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidAssignment);
        }

		std::string str = next.value().GetValueString();
        // 标识符是常量吗？
		if(isConstant(str))
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrAssignToConstant);
		// 标识符声明过吗？
        else if(!isDeclared(str))
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNotDeclared);

        // 判断下一个是否为 '='
        next = nextToken();
        if (next.value().GetType() != TokenType::EQUAL_SIGN){
            return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidAssignment);
        }

        // 解析表达式
        auto err=analyseExpression();
        if (err.has_value())
            return err;

		// 需要生成指令吗？
		// 需要替换掉map中的对应变量的值
		// 情况一：
		// 如果是没有初始化过的，要从未初始化_uninitialized_vars map中删除，
		// 同时添加到已经初始化的_vars map中
		int32_t in = getIndex(str);
        if(isUninitializedVariable(str)){
            _vars.insert(std::pair<std::string,int32_t >(str,in));
            _uninitialized_vars.erase(str);
        }
        // 情况二：
        // 如果是初始化了的，直接添加命令，将位于0偏移位置的数值加载到标识符对应位置
        _instructions.emplace_back(STO,in);
		return {};
	}

	// <输出语句> ::= 'print' '(' <表达式> ')' ';'
	std::optional<CompilationError> Analyser::analyseOutputStatement() {
		// 如果之前 <语句序列> 的实现正确，这里第一个 next 一定是 TokenType::PRINT
		auto next = nextToken();

		// '('
		next = nextToken();
		if (!next.has_value() || next.value().GetType() != TokenType::LEFT_BRACKET)
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidPrint);

		// <表达式>
		auto err = analyseExpression();
		if (err.has_value())
			return err;

		// ')'
		next = nextToken();
		if (!next.has_value() || next.value().GetType() != TokenType::RIGHT_BRACKET)
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidPrint);

		// ';'
		next = nextToken();
		if (!next.has_value() || next.value().GetType() != TokenType::SEMICOLON)
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoSemicolon);

		// 生成相应的指令 WRT
		_instructions.emplace_back(Operation::WRT, 0);
		return {};
	}

	// <项> :: = <因子>{ <乘法型运算符><因子> }
	// 需要补全 已补全
	std::optional<CompilationError> Analyser::analyseItem() {
		// 可以参考 <表达式> 实现

        // <因子>
        auto err = analyseFactor();
        if (err.has_value())
            return err;

        // {<乘法型运算符><因子>}
        while (true) {
            // 预读
            auto next = nextToken();
            if (!next.has_value())
                return {};
            auto type = next.value().GetType();
            if (type != TokenType::MULTIPLICATION_SIGN && type != TokenType::DIVISION_SIGN) {
                unreadToken();
                return {};
            }

            // <因子>
            err = analyseFactor();
            if (err.has_value())
                return err;

            // 根据结果生成指令
            if (type == TokenType::MULTIPLICATION_SIGN)
                _instructions.emplace_back(Operation::MUL, 0);
            else if (type == TokenType::DIVISION_SIGN)
                _instructions.emplace_back(Operation::DIV, 0);
        }
        return {};
	}

	// <因子> ::= [<符号>]( <标识符> | <无符号整数> | '('<表达式>')' )
	// 需要补全 已补全
	std::optional<CompilationError> Analyser::analyseFactor() {
		// [<符号>]
		auto next = nextToken();
		auto prefix = 1;
		if (!next.has_value())
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteExpression);
		if (next.value().GetType() == TokenType::PLUS_SIGN)
			prefix = 1;
		else if (next.value().GetType() == TokenType::MINUS_SIGN) {
			prefix = -1;
			_instructions.emplace_back(Operation::LIT, 0);
		}
		else
			unreadToken();

		// 预读
		next = nextToken();
		if (!next.has_value())
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteExpression);
		switch (next.value().GetType()) {
			// 这里和 <语句序列> 类似，需要根据预读结果调用不同的子程序
			// 但是要注意 default 返回的是一个编译错误
            case TokenType::IDENTIFIER:{
                // 1. 先判断是否声明过+是否初始化过
                // 2. 然后从map中取出这个值的位置
                // 3. 添加指令，将这个值压入栈中
                std::string str = next.value().GetValueString();
                // 标识符是常量吗？
                if(!isDeclared(str))
                    return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNotDeclared);
                if(isUninitializedVariable(str))
                    return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNotInitialized);
                int32_t in = getIndex(str);
                _instructions.emplace_back(Operation::LOD, in);
                break;
            }
            case TokenType::UNSIGNED_INTEGER:{
                // 1. 直接转化为int32_t
                // 2. 添加指令，将这个值压入栈中
                int32_t val;
                std::stringstream ss;
                ss << next.value().GetValueString();
                ss >> val;
                _instructions.emplace_back(Operation::LIT, val);
                break;
            }
            case TokenType::LEFT_BRACKET:{
                auto err=analyseExpression();
                if(err.has_value())
                    return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteExpression);
                next=nextToken();
                if(!next.has_value()||next.value().GetType()!=TokenType::RIGHT_BRACKET)
                    return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteExpression);
                break;
            }
            default:
                return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteExpression);
		}

		// 取负
		if (prefix == -1)
			_instructions.emplace_back(Operation::SUB, 0);
		return {};
	}

	std::optional<Token> Analyser::nextToken() {
		if (_offset == _tokens.size())
			return {};
		// 考虑到 _tokens[0..._offset-1] 已经被分析过了
		// 所以我们选择 _tokens[0..._offset-1] 的 EndPos 作为当前位置
		_current_pos = _tokens[_offset].GetEndPos();
		return _tokens[_offset++];
	}

	void Analyser::unreadToken() {
		if (_offset == 0)
			DieAndPrint("analyser unreads token from the begining.");
		_current_pos = _tokens[_offset - 1].GetEndPos();
		_offset--;
	}

	void Analyser::_add(const Token& tk, std::map<std::string, int32_t>& mp) {
		if (tk.GetType() != TokenType::IDENTIFIER)
			DieAndPrint("only identifier can be added to the table.");
		mp[tk.GetValueString()] = _nextTokenIndex;
		_nextTokenIndex++;
	}

	void Analyser::addVariable(const Token& tk) {
		_add(tk, _vars);
	}

	void Analyser::addConstant(const Token& tk) {
		_add(tk, _consts);
	}

	void Analyser::addUninitializedVariable(const Token& tk) {
		_add(tk, _uninitialized_vars);
	}

	int32_t Analyser::getIndex(const std::string& s) {
		if (_uninitialized_vars.find(s) != _uninitialized_vars.end())
			return _uninitialized_vars[s];
		else if (_vars.find(s) != _vars.end())
			return _vars[s];
		else
			return _consts[s];
	}

	bool Analyser::isDeclared(const std::string& s) {
		return isConstant(s) || isUninitializedVariable(s) || isInitializedVariable(s);
	}

	bool Analyser::isUninitializedVariable(const std::string& s) {
		return _uninitialized_vars.find(s) != _uninitialized_vars.end();
	}
	bool Analyser::isInitializedVariable(const std::string&s) {
		return _vars.find(s) != _vars.end();
	}

	bool Analyser::isConstant(const std::string&s) {
		return _consts.find(s) != _consts.end();
	}
}