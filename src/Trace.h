/*
 * Copyright 2013 Saminda Abeyruwan (saminda@cs.miami.edu)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Trace.h
 *
 *  Created on: Aug 19, 2012
 *      Author: sam
 */

#ifndef TRACE_H_
#define TRACE_H_
#include <iostream>
#include "Vector.h"
#include "Math.h"

namespace RLLib
{

template<class T>
class Trace
{
  public:
    virtual ~Trace()
    {
    }
    virtual void update(const double& lambda, const Vector<T>* phi) =0;
    virtual void multiplyToSelf(const double& factor) =0;
    virtual void setEntry(const int& index, const T& value) =0;
    virtual void clear() =0;
    virtual const Vector<T>* vect() const =0;
};

template<class T>
class ATrace: public Trace<T>
{
  protected:
    double defaultThreshold;
    double threshold;
    SparseVector<T>* vector;
  public:
    ATrace(const int& numFeatures, const double& threshold = 1e-8) :
        defaultThreshold(threshold), threshold(threshold), vector(new SVector<T>(numFeatures))
    {
    }
    virtual ~ATrace()
    {
      delete vector;
    }

  private:
    virtual void clearBelowThreshold()
    {
      const T* values = vector->getValues();
      const int* indexes = vector->nonZeroIndexes();
      int i = 0;
      while (i < vector->nonZeroElements())
      {
        T absValue = fabs(values[i]);
        if (absValue <= threshold)
          vector->removeEntry(indexes[i]);
        else
          i++;
      }
    }

  public:
    virtual void updateVector(const double& lambda, const Vector<T>* phi)
    {
      vector->mapMultiplyToSelf(lambda);
      vector->addToSelf(phi);
    }

    void update(const double& lambda, const Vector<T>* phi)
    {
      updateVector(lambda, phi);
      adjustUpdate();
      clearBelowThreshold();
    }

    void multiplyToSelf(const double& factor)
    {
      vector->mapMultiplyToSelf(factor);
    }

    void setEntry(const int& index, const T& value)
    {
      vector->setEntry(index, value);
    }

    void clear()
    {
      vector->clear();
      threshold = defaultThreshold;
    }
    const Vector<T>* vect() const
    {
      return vector;
    }

  protected:
    virtual void adjustUpdate()
    { // Nothing to be adjusted.
    }

};

template<class T>
class RTrace: public ATrace<T>
{
  public:
    RTrace(const int& capacity, const double& threshold = 1e-8) :
        ATrace<T>(capacity, threshold)
    {
    }

    virtual ~RTrace()
    {
    }

  private:
    void replaceWith(const Vector<T>* x)
    { // FixMe:
      const SparseVector<T>& phi = *(const SparseVector<T>*) x;
      const int* indexes = phi.nonZeroIndexes();
      for (const int* index = indexes; index < indexes + phi.nonZeroElements(); ++index)
        ATrace<T>::vector->setEntry(*index, phi.getEntry(*index));
    }
  public:
    virtual void updateVector(const double& lambda, const Vector<T>* phi)
    {
      ATrace<T>::vector->mapMultiplyToSelf(lambda);
      replaceWith(phi);
    }

};

template<class T>
class AMaxTrace: public ATrace<T>
{
  protected:
    double maximumValue;
  public:
    AMaxTrace(const int& capacity, const double& threshold = 1e-8, const double& maximumValue = 1.0) :
        ATrace<T>(capacity, threshold), maximumValue(maximumValue)
    {
    }

    virtual ~AMaxTrace()
    {
    }

  public:

    virtual void adjustUpdate()
    {
      const T* values = ATrace<T>::vector->getValues();
      const int* indexes = ATrace<T>::vector->nonZeroIndexes();

      for (int i = 0; i < ATrace<T>::vector->nonZeroElements(); i++)
      {
        T absValue = fabs(values[i]);
        if (absValue > maximumValue)
          ATrace<T>::vector->setEntry(indexes[i], Signum::valueOf(absValue) * maximumValue);
      }
    }

};

template<class T>
class MaxLengthTrace: public Trace<T>
{
  protected:
    Trace<T>* trace;
    int maximumLength;

  public:
    MaxLengthTrace(Trace<T>* trace, const int& maximumLength) :
        trace(trace), maximumLength(maximumLength)
    {
    }

    virtual ~MaxLengthTrace()
    {
    }

  private:
    void controlLength()
    { // FixMe:
      const SparseVector<T>* v = (const SparseVector<T>*) trace->vect();
      if (v->nonZeroElements() < maximumLength)
        return;
      while (v->nonZeroElements() > maximumLength)
      {
        const T* values = v->getValues();
        const int* indexes = v->nonZeroIndexes();
        T minValue = values[0];
        int minIndex = indexes[0];
        for (int i = 1; i < v->nonZeroElements(); i++)
        {
          if (values[i] < minValue)
          {
            minValue = values[i];
            minIndex = indexes[i];
          }
        }
        trace->setEntry(minIndex, 0);
      }
    }

  public:

    void update(const double& lambda, const Vector<T>* phi)
    {
      trace->update(lambda, phi);
      controlLength();
    }

    void multiplyToSelf(const double& factor)
    {
      trace->multiplyToSelf(factor);
    }

    void setEntry(const int& index, const T& value)
    {
      trace->setEntry(index, value);
    }

    void clear()
    {
      trace->clear();
    }
    const Vector<T>* vect() const
    {
      return trace->vect();
    }
};

template<class T>
class Traces
{
  protected:
    typename std::vector<Trace<T>*> traces;
  public:
    typedef typename std::vector<Trace<T>*>::iterator iterator;
    typedef typename std::vector<Trace<T>*>::const_iterator const_iterator;

    Traces()
    {
    }

    ~Traces()
    {
      traces.clear();
    }

    Traces(const Traces<T>& that)
    {
      for (typename Traces<T>::iterator iter = that.begin(); iter != that.end(); ++iter)
        traces.push_back(*iter);
    }

    Traces<T>& operator=(const Traces<T>& that)
    {
      if (this != that)
      {
        traces.clear();
        for (typename Traces<T>::iterator iter = that.begin(); iter != that.end(); ++iter)
          traces.push_back(*iter);
      }
      return *this;
    }

    void push_back(Trace<T>* trace)
    {
      traces.push_back(trace);
    }

    iterator begin()
    {
      return traces.begin();
    }

    const_iterator begin() const
    {
      return traces.begin();
    }

    iterator end()
    {
      return traces.end();
    }

    const_iterator end() const
    {
      return traces.end();
    }

    int dimension() const
    {
      return traces.size();
    }

    Trace<T>* at(const int& index)
    {
      return traces.at(index);
    }

    const Trace<T>* at(const int& index) const
    {
      return traces.at(index);
    }

    void clear()
    {
      for (typename Traces<T>::iterator iter = begin(); iter != end(); ++iter)
        (*iter)->clear();
    }
};

} // namespace RLLib

#endif /* TRACE_H_ */
