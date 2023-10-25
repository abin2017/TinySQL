#ifndef _SQL_GRAMMER_H__
#define _SQL_GRAMMER_H__


/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    UNKNOWN_TOKEN = -1,
    CREATE = 258,
    DELETE = 259,
    DROP = 260,
    DATABASE = 261,
    TABLE = 262,
    SELECT = 263,
    INSERT_INTO = 264,
    UPDATE = 265,
    SET = 266,
    FROM = 267,
    WHERE = 268,
    AS = 269,
    ALL = 270,
    ANY = 271,
    ON = 272,
    ASC = 273,
    DESC = 274,
    DATATYPE = 275,
    NUMBER = 276,
    STRING = 277,
    PATTERN = 278,
    IDENTIFIER = 279,
    SELECTALL = 280,
    COMMA = 281,
    DISTINCT = 282,
    INNER_JOIN = 283,
    LEFT_JOIN = 284,
    RIGHT_JOIN = 285,
    FULL_JOIN = 286,
    COUNT = 287,
    AVERAGE = 288,
    SUM = 289,
    HAVING = 290,
    EXISTS = 291,
    LIMIT = 292,
    IN = 293,
    NOT_IN = 294,
    MINIMUM = 295,
    MAXIMUM = 296,
    UNION = 297,
    UNION_ALL = 298,
    VALUES = 299,
    RELATIONAL_OPERATOR = 300,
    OR = 301,
    AND = 302,
    EQUALITY_OPERATOR = 303,
    NOT = 304,
    IS_NULL = 305,
    IS_NOT_NULL = 306,
    LIKE = 307,
    NOT_LIKE = 308,
    BETWEEN = 309,
    NOT_BETWEEN = 310,
    ORDER_BY = 311,
    GROUP_BY = 312
  };
#endif


typedef struct Node {
    struct Node* child;
    struct Node* sibling;
    const char *str;
    char *value;
    enum yytokentype token;
}stg_node_t;


stg_node_t * sql_tiny_grammer_api_parse(char *sql);

void sql_tiny_grammer_api_end();

void sql_tiny_grammer_api_destroy();


#endif