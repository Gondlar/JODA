//
// Created by Nico on 13/05/2019.
//

#ifndef JODA_SIMILARITYSCHEDULER_H
#define JODA_SIMILARITYSCHEDULER_H

#include <joda/container/JSONContainer.h>
#include <joda/container/ContainerFlags.h>
#include <joda/storage/JSONStorage.h>
#include <joda/config/config.h>
#include <rapidjson/istreamwrapper.h>
#include "measures/IJSONSimilarityMeasure.h"

/**
 * A document scheduler based on document similarity.
 * Similar documents are grouped into the same containers.
 * @tparam SIM The similarity measure to use.
 */
template<typename SIM>
class SimilarityScheduler {
 public:
  typedef size_t ContainerIdentifier;

  /**
   * Constructs a scheduler for use within the parsers.
   * @param queue  The container queue to use to pass finalized containers through.
   * @param contSize The size of newly constructed containers.
   */
  explicit SimilarityScheduler(JsonContainerQueue::queue_t *queue, size_t contSize =0);

  /**
   * Uses the string representation of a document to get the container in which the document should be stored.
   * @param raw The document in string representation (before parsing)
   * @return An identifier representing the container in which the document will be stored.
   */
  ContainerIdentifier getContainerForDoc(std::string& raw);

  /**
 * Uses the document to get the container in which the document should be stored.
 * @param doc The document
 * @return An identifier representing the container in which the document will be stored.
 */
  ContainerIdentifier getContainerForDoc(const RJDocument &doc);

  /**
   * Uses the stream of a document to get the container in which the document should be stored.
   * @param stream The document in a stream (before parsing)
   * @return An identifier representing the container in which the document will be stored.
   */
  ContainerIdentifier getContainerForDoc(rapidjson::IStreamWrapper &stream);

  /**
   * Returns a document initialized with the allocator of the container represented by id
   * @param id The id representing the container
   * @return A new document
   */
  std::shared_ptr<RJDocument> getNewDoc(ContainerIdentifier id);

  /**
   * Schedules the document into the container given by id.
   * @param id The id of the container
   * @param doc The document to schedule
   * @param origin The origin of the document
   * @param size The size of the document (bytes)
   */
  void scheduleDocument(ContainerIdentifier id, std::shared_ptr<RJDocument> &&doc,
                        std::unique_ptr<IOrigin> &&origin,
                        size_t size);
  virtual ~SimilarityScheduler() = default;

  /**
   * Finalizes the Scheduler.
   * Has to be called before deconstructing it.
   */
  void finalize();

 private:
  typedef std::pair<std::unique_ptr<JSONContainer>, typename similarityRepresentation<SIM>::Representation > sCont;
  std::vector<sCont> container;
  JsonContainerQueue::queue_t* queue;
  size_t contSize;
  std::unique_ptr<JSONContainer> createContainer(size_t contSize) const;

};

template<typename SIM>
std::unique_ptr<JSONContainer> SimilarityScheduler<SIM>::createContainer(size_t contSize) const {
  if (contSize == 0)
    return std::make_unique<JSONContainer>(); //& Create new one
  else
    return std::make_unique<JSONContainer>(contSize); //& Create new one
}


template<typename SIM>
std::shared_ptr<RJDocument> SimilarityScheduler<SIM>::getNewDoc(ContainerIdentifier id) {
  return std::make_shared<RJDocument>(container[id].first->getAlloc());
}

template<typename SIM>
void SimilarityScheduler<SIM>::scheduleDocument(ContainerIdentifier id,
                                                std::shared_ptr<RJDocument> &&doc,
                                                std::unique_ptr<IOrigin> &&origin,
                                                size_t size) {

      container[id].first->insertDoc(JSONStorage::getID(), std::move(doc), std::move(origin));
      if (!(container[id].first->hasSpace(0) || container[id].first->size() == 0)) {
        container[id].first->finalize();
        DCHECK(container[id].first != nullptr);
        queue->send(std::move(container[id].first)); //Enqueue
        DCHECK(container[id].first == nullptr);
        container[id].first = createContainer(contSize);
        DCHECK(container[id].first != nullptr);
      }

}

template<typename SIM>
SimilarityScheduler<SIM>::SimilarityScheduler(JsonContainerQueue::queue_t *queue, size_t contSize) : contSize(contSize), queue(queue) {
}

template<typename SIM>
typename SimilarityScheduler<SIM>::ContainerIdentifier SimilarityScheduler<SIM>::getContainerForDoc(std::string& raw) {
  SIM measure;
  similarityRepresentation<SIM> simRep;
  DCHECK(simRep.is_implemented) << "Function called with wrong Similarity measure";
  auto docRep = simRep.getRepresentation(raw);

  if (!container.empty()) {
    double maxSim = 0;
    int maxSimi = -1;
    for (int i = 0; i < container.size(); ++i) {
      double sim = measure.measure(docRep, container[i].second);
      if (sim > maxSim) {
        maxSim = sim;
        maxSimi = i;
      }
    }
    if (maxSim >= config::sim_min_similarity) {
      return maxSimi;
    }
  }
  container.emplace_back(createContainer(contSize), std::move(docRep));
  return container.size() - 1;
}

template<typename SIM>
typename SimilarityScheduler<SIM>::ContainerIdentifier SimilarityScheduler<SIM>::getContainerForDoc(const RJDocument &doc) {
  SIM measure;
  similarityRepresentation<SIM> simRep;
  DCHECK(simRep.is_implemented) << "Function called with wrong Similarity measure";
  auto docRep = simRep.getRepresentation(doc);

  if (!container.empty()) {
    double maxSim = 0;
    int maxSimi = -1;
    for (int i = 0; i < container.size(); ++i) {
      double sim = measure.measure(docRep, container[i].second);
      if (sim > maxSim) {
        maxSim = sim;
        maxSimi = i;
      }
    }
    if (maxSim >= config::sim_min_similarity) {
      return maxSimi;
    }
  }
  container.emplace_back(createContainer(contSize), std::move(docRep));
  return container.size() - 1;
}

template<typename SIM>
typename SimilarityScheduler<SIM>::ContainerIdentifier SimilarityScheduler<SIM>::getContainerForDoc(rapidjson::IStreamWrapper &stream) {
  SIM measure;
  similarityRepresentation<SIM> simRep;
  DCHECK(simRep.is_implemented) << "Function called with wrong Similarity measure";
  auto docRep = simRep.getRepresentation(stream);

  if(!container.empty()){
    double maxSim = 0;
    int maxSimi = -1;
    for (int i = 0; i < container.size(); ++i) {
      double sim = measure.measure(docRep, container[i].second);
      if(sim > maxSim){
        maxSim=sim;
        maxSimi=i;
      }
    }
    if(maxSim >= config::sim_min_similarity){
      return maxSimi;
    }
  }
  container.emplace_back(createContainer(contSize), std::move(docRep));
  return container.size()-1;
}

template<typename SIM>
void SimilarityScheduler<SIM>::finalize() {
  //TODO Merge small container

  for (auto &&currentSimContainer : container) {
    auto& currentContainer = currentSimContainer.first;
    if (currentContainer->size() > 0) {
      DCHECK(currentContainer != nullptr);
      currentContainer->finalize();
      if (!config::storeJson)
        currentContainer->removeDocuments();
      queue->send(std::move(currentContainer)); //Enqueue
      DCHECK(currentContainer == nullptr);
    }
  }
  queue->producerFinished();
}

#endif //JODA_SIMILARITYSCHEDULER_H
