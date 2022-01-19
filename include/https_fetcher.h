#pragma once

typedef struct PRWfetcher PRWfetcher;

PRWfetcher* prwfFetch(const char* host, const char* dir);

PRWfetcher* prwfFetchURL(const char* url);

int prwfFetchComplete(PRWfetcher* fetcher);

const char* prwfFetchString(PRWfetcher* fetcher, int* length);

void prwfFetchWait(PRWfetcher* fetcher);

void prwfFreeFetcher(PRWfetcher*);