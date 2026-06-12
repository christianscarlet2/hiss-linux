#ifndef HISS_STUB_LIBPQ
#define HISS_STUB_LIBPQ
typedef struct pg_conn PGconn;
typedef struct pg_result PGresult;
typedef enum { CONNECTION_OK, CONNECTION_BAD } ConnStatusType;
typedef enum { PGRES_EMPTY_QUERY, PGRES_COMMAND_OK, PGRES_TUPLES_OK, PGRES_COPY_OUT, PGRES_COPY_IN,
               PGRES_BAD_RESPONSE, PGRES_NONFATAL_ERROR, PGRES_FATAL_ERROR } ExecStatusType;
inline PGconn* PQconnectdb(const char*) { return nullptr; }
inline void PQfinish(PGconn*) {}
inline ConnStatusType PQstatus(const PGconn*) { return CONNECTION_BAD; }
inline PGresult* PQexec(PGconn*, const char*) { return nullptr; }
inline ExecStatusType PQresultStatus(const PGresult*) { return PGRES_FATAL_ERROR; }
inline int PQntuples(const PGresult*) { return 0; }
inline int PQnfields(const PGresult*) { return 0; }
inline char* PQgetvalue(const PGresult*, int, int) { return (char*)""; }
inline void PQclear(PGresult*) {}
inline char* PQerrorMessage(const PGconn*) { return (char*)""; }
inline char* PQfname(const PGresult*, int) { return (char*)""; }
inline void PQreset(PGconn*) {}
inline int PQgetisnull(const PGresult*, int, int) { return 1; }
inline int PQgetlength(const PGresult*, int, int) { return 0; }
#endif
