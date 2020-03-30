#include "HeuristicDecoder.h"

const int K_MAX = 50;

HeuristicDecoder *heu;
vector<Link> nonScheduledLinks;
double maximumTime;
clock_t startTime;

bool isStoppingCriteriaReached() {
  return (((double) (clock() - startTime)) / CLOCKS_PER_SEC) >= maximumTime;
}

int betaAddDrop(const Solution current, Link link) {
  vector<int> channels = current.getScheduledChannels();
  int bestChannel = -1; //TODO: remember to update bestChannel value.

  for (int ch : channels) {
    Solution aux(current);
    Link copyLink(link);
    copyLink.setChannel(ch);

    aux.insert(copyLink);

    if (aux > current) {
      bestChannel = ch;
    }
  }

  return bestChannel;
}

void betaAddDrop(Solution &current, int beta = 1) {
  for (int i = 0; i < beta; i++) {
    int rndIndex = rng.randInt(nonScheduledLinks.size() - 1);
    int bestChannel = betaAddDrop(current, nonScheduledLinks[rndIndex]);
    assert(bestChannel != -1);

    Link toInsert(nonScheduledLinks[rndIndex]);
    toInsert.setChannel(bestChannel);
    current.insert(toInsert);

    swap(nonScheduledLinks[rndIndex], nonScheduledLinks.back());
    nonScheduledLinks.pop_back();
  }
}

void betaReinsert(Solution &current, int beta = 1) {
  unordered_set<int> usedLinks;
  for (int i = 0; i < beta; i++) {
    int rndIndex = rng.randInt(current.getNumberOfScheduledLinks() - 1);

    while (usedLinks.count(rndIndex))
      rndIndex = rng.randInt(current.getNumberOfScheduledLinks() - 1);

    Link rmvLink = current.removeLinkByIndex(rndIndex);
    int bestChannel = betaAddDrop(current, rmvLink);
    assert(bestChannel != -1);

    Link toInsert(nonScheduledLinks[rndIndex]);
    toInsert.setChannel(bestChannel);
    current.insert(toInsert);
  }
}

Solution solve(Solution S, int channel) {
  //TODO: best <- canal.throughput (?????)
  double best = S.getChannelThroughput(channel);

  if (whichBw(channel) > 20) {
    //TODO
//    double bestC1 = solve(S, mapChtoCh[channel].first); //FIXME: it's not double, but Solution
//    double bestC2 = solve(S, mapChtoCh[channel].second);

//    if (bestC1 + bestC2 > best) {
//      best = bestC1 + bestC2;
//      TODO: canal.inSolution = false
//    }
  }

  return Solution();
}

bool allChannels20MHz(const Solution &check) {
  for (const int &x : check.getScheduledChannels()) {
    if (whichBw(x) > 20)
      return false;
  }

  return true;
}

Solution convert(const Solution &aux) { //TODO: colocar todo mundo em canal de 20MHz
  Solution ret(aux);

  while (!allChannels20MHz(ret)) {
    for (Link &x : ret.getScheduledLinks()) {
      if (whichBw(x.getChannel()) > 20) {
        split(ret, ret, x.getChannel(), true);
        break;
      }
    }
  }
  return ret;
}

Solution localSearch(Solution current) {
  bool improve = true;
  Solution S_star = convert(current);
  Solution S(S_star), S_2(S_star);

  while (improve) {
    for (Link active : current.getScheduledLinks()) {
      Solution SCopy = S;
      Solution S_1 = SCopy;
      S_1.removeLink(active);
      for (int channel20 : channels20MHz) {
        Link aux(active);
        aux.setChannel(channel20);
        S_1.insert(aux);
        solve(S_1, channel20);

        S_2 = (S_1 > S_2) ? S_1 : S_2;
      }

      if (S_2 > S_star) {
        S_star = S_2;
        S = S_2;
      } else {
        improve = false;
      }
    }
  }

  return S; //FIXME: o que eh para retornar?
}

Solution pertubation(Solution S, int k, const int NUMBER_OF_LINKS) {
  double threeshold = rng.rand();
  if (threeshold >= .5) {
    betaAddDrop(S, NUMBER_OF_LINKS * ((k * 1.0) / 100.0));
  } else {
    betaReinsert(S, NUMBER_OF_LINKS * ((k * 1.0) / 100.0));
  }
  return S;
}

void init() {
#ifdef DEBUG_CLION
  puts("WITH DEBUG");
  freopen("/Users/jjaneto/Downloads/codes_new/BRKGA_FF_Best/Instancias/D250x250/U_8/U_8_1.txt", "r", stdin);
#endif

  heu = new HeuristicDecoder();
  for (int i = 0; i < heu->getQuantConnections(); i++) {
    nonScheduledLinks.emplace_back(Link(i));
  }
  maximumTime = 10;
}

int main(int argc, char *argv[]) {
  init();
  const int NUMBER_OF_LINKS = heu->getQuantConnections();
  startTime = clock();
  //--------------------------------------------------------
  Solution S_dummy = heu->generateSolution();
  Solution S = localSearch(S_dummy);
  while (!isStoppingCriteriaReached()) {
    int k = 1;
    while (k < K_MAX) {
      Solution S_1 = pertubation(S, k, NUMBER_OF_LINKS);
      Solution S_2 = localSearch(S_1);

      if (S_2.getObjective() > S_1.getObjective()) {
        S = S_2;
        k = 1;
      } else {
        k++; //TODO: isso mesmo?
      }
    }
  }

  delete heu;
  return 0;
}