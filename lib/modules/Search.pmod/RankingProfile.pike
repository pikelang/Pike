array(int) field_ranking;
array(int) proximity_ranking;
int cutoff;

void create(Search.Database.MySQL db, void|mapping _field_ranking)
{
  field_ranking=allocate(66);
  field_ranking[0]=17;
  field_ranking[2]=147;

  prox_ranking=allocate(8);
  for(int i=0; i<8; i++)
    prox_ranking[i]=8-i;
}
