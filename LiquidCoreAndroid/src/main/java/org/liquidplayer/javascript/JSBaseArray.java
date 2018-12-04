/*
 * Copyright (c) 2014 - 2018 Eric Lange
 *
 * Distributed under the MIT License.  See LICENSE.md at
 * https://github.com/LiquidPlayer/LiquidCore for terms and conditions.
 */
package org.liquidplayer.javascript;

import android.support.annotation.NonNull;

import java.lang.reflect.Array;
import java.util.Collection;
import java.util.Iterator;
import java.util.List;
import java.util.ListIterator;
import java.util.NoSuchElementException;

/**
 * A convenience class for handling JavaScript arrays.  Implements java.util.List interface for
 * simple integration with Java methods.
 *
 */
@SuppressWarnings("WeakerAccess,SameParameterValue")
public abstract class JSBaseArray<T> extends JSFunction implements List<T> {

    Class<T> mType;
    int mLeftBuffer = 0;
    int mRightBuffer = 0;
    JSBaseArray<T> mSuperList = null;

    JSBaseArray(JNIJSObject valueRef, JSContext ctx, Class<T> cls) {
        super(valueRef,ctx);
        mType = cls;
    }
    JSBaseArray(JSBaseArray<T> superList, int leftBuffer, int rightBuffer, Class<T> cls) {
        mType = cls;
        mLeftBuffer = leftBuffer;
        mRightBuffer = rightBuffer;
        context = superList.context;
        valueRef = superList.valueRef();
        mSuperList = superList;
    }
    JSBaseArray(JSContext ctx, Class<T> cls) {
        context = ctx;
        mType = cls;
    }


    /**
     * Converts to a static array with elements of class 'clazz'
     * @param clazz   The class to convert the elements to (Integer.class, Double.class,
     *                String.class, JSValue.class, etc.)
     * @return The captured static array
     * @since 0.1.0
     */
    public Object[] toArray(Class clazz) {
        int count = size();

        Object [] array = (Object[]) Array.newInstance(clazz,count);
        for (int i=0; i<count; i++) {
            array[i] = elementAtIndex(i).toJavaObject(clazz);
        }
        return array;
    }

    /**
     * Extracts Java JSValue array from JavaScript array
     * @see List#toArray()
     * @return JavaScript array as Java array of JSValues
     */
    @Override @NonNull
    public Object [] toArray() {
        return toArray(mType);
    }

    /**
     * Gets JSValue at 'index'
     * @see List#get(int)
     * @param index  Index of the element to get
     * @return  The JSValue at index 'index'
     * @since 0.1.0
     */
    @Override
    @SuppressWarnings("unchecked")
    public T get(final int index) {
        int count = size();
        if (index >= count) {
            throw new ArrayIndexOutOfBoundsException();
        }
        return (T) elementAtIndex(index).toJavaObject(mType);
    }

    /**
     * Adds a JSValue to the end of an array.  The Java Object is converted to a JSValue.
     * @see List#add(Object)
     * @param val  The Java object to add to the array, will get converted to a JSValue
     * @since 0.1.0
     */
    @Override
    public boolean add(final T val) {
        int count = size();
        elementAtIndex(count,val);
        return true;
    }

    /**
     * @see List#size()
     * @since 0.1.0
     */
    @Override
    public int size() {
        if (mSuperList == null) {
            return property("length").toNumber().intValue();
        } else {
            return Math.max(0, mSuperList.size() - mLeftBuffer - mRightBuffer);
        }
    }

    protected JSValue arrayElement(final int index) {
        return propertyAtIndex(index);
    }

    protected void arrayElement(final int index, final T value) {
        propertyAtIndex(index,value);
    }

    JSValue elementAtIndex(final int index) {
        if (mSuperList == null)
            return arrayElement(index);
        else
            return mSuperList.elementAtIndex(index + mLeftBuffer);
    }

    void elementAtIndex(final int index, final T value) {
        if (mSuperList == null)
            arrayElement(index, value);
        else
            mSuperList.elementAtIndex(index + mLeftBuffer, value);
    }

    /**
     * @see List#isEmpty() ()
     * @since 0.1.0
     */
    @Override
    public boolean isEmpty() {
        return (size() == 0);
    }

    /**
     * @see List#contains(Object)  ()
     * @since 0.1.0
     */
    @Override
    public boolean contains(final Object object) {
        for (int i=0; i<size(); i++) {
            if(get(i).equals(object))
                return true;
        }
        return false;
    }

    private class ArrayIterator implements ListIterator<T> {
        private int current = 0;
        private Integer modifiable = null;

        public ArrayIterator() {
            this(0);
        }
        public ArrayIterator(int index) {
            if (index > size()) index = size();
            if (index < 0) index = 0;
            current = index;
        }

        @Override
        public boolean hasNext() {
            return (current < size());
        }

        @Override
        public boolean hasPrevious() {
            return (current > 0);
        }

        @Override
        public T next() {
            if (!hasNext())
                throw new NoSuchElementException();
            modifiable = current;
            return get(current++);
        }

        @Override
        public T previous() {
            if (!hasPrevious())
                throw new NoSuchElementException();
            modifiable = --current;
            return get(current);
        }

        @Override
        public void remove() {
            if (modifiable==null)
                throw new NoSuchElementException();

            JSBaseArray.this.remove(modifiable.intValue());
            current = modifiable;
            modifiable = null;
        }

        @Override
        public int nextIndex() {
            return current;
        }

        @Override
        public int previousIndex() {
            return current - 1;
        }

        @Override
        public void set(T value) {
            if (modifiable==null)
                throw new NoSuchElementException();

            JSBaseArray.this.set(modifiable,value);
        }

        @Override
        public void add(T value) {
            JSBaseArray.this.add(current++,value);
            modifiable = null;
        }
    }

    /**
     * @see List#iterator()
     * @since 0.1.0
     */
    @Override
    public @NonNull
    Iterator<T> iterator() {
        return new ArrayIterator();
    }

    /**
     * @see List#toArray(Object[])
     * @since 0.1.0
     */
    @Override
    @SuppressWarnings("unchecked")
    @NonNull
    public <U> U[] toArray(final @NonNull U[] elemArray) {
        if (size() > elemArray.length) {
            return (U[])toArray();
        }
        ArrayIterator iterator = new ArrayIterator();
        int index = 0;
        while (iterator.hasNext()) {
            Object next = iterator.next();
            elemArray[index++] = (U)next;
        }
        for (int i = index; i < elemArray.length; i++) {
            elemArray[i] = null;
        }
        return elemArray;
    }

    /**
     * @see List#remove(Object)
     * @since 0.1.0
     */
    @Override
    public boolean remove(final Object object) {
        ArrayIterator listIterator = new ArrayIterator();
        while (listIterator.hasNext()) {
            if (listIterator.next().equals(object)) {
                listIterator.remove();
                return true;
            }
        }
        return false;
    }

    /**
     * @see List#containsAll(Collection)
     * @since 0.1.0
     */
    @Override
    public boolean containsAll(final @NonNull Collection<?> collection) {
        for (Object item : collection.toArray()) {
            if (!contains(item)) return false;
        }
        return true;
    }

    /**
     * @see List#addAll(Collection)
     * @since 0.1.0
     */
    @Override
    public boolean addAll(final @NonNull Collection<? extends T> collection) {
        return addAll(size(), collection);
    }

    /**
     * @see List#addAll(int, Collection)
     * @since 0.1.0
     */
    @Override
    @SuppressWarnings("unchecked")
    public boolean addAll(final int index, final @NonNull Collection<? extends T> collection) {
        int i = index;
        for (Object item : collection.toArray()) {
            add(i++,(T)item);
        }
        return true;
    }

    /**
     * @see List#removeAll(Collection)
     * @since 0.1.0
     */
    @Override
    public boolean removeAll(final @NonNull Collection<?> collection) {
        boolean any = false;
        ListIterator<T> listIterator = listIterator();
        while (listIterator.hasNext()) {
            T compare = listIterator.next();
            for (Object element : collection) {
                if (compare.equals(element)) {
                    listIterator.remove();
                    any = true;
                    break;
                }
            }
        }
        return any;
    }

    /**
     * @see List#retainAll(Collection)
     * @since 0.1.0
     */
    @Override
    public boolean retainAll(final @NonNull Collection<?> collection) {
        boolean any = false;
        ListIterator<T> listIterator = listIterator();
        while (listIterator.hasNext()) {
            T compare = listIterator.next();
            boolean remove = true;
            for (Object element : collection) {
                if (compare.equals(element)) {
                    remove = false;
                    break;
                }
            }
            if (remove) {
                listIterator.remove();
                any = true;
            }
        }
        return any;
    }

    /**
     * @see List#clear()
     * @since 0.1.0
     */
    @Override
    public void clear() {
        for (int i=size(); i > 0; --i) {
            remove(i-1);
        }
    }

    /**
     * @see List#set(int, Object)
     * @since 0.1.0
     */
    @Override
    @SuppressWarnings("unchecked")
    public T set(final int index, final T element) {
        int count = size();
        if (index >= count) {
            throw new ArrayIndexOutOfBoundsException();
        }
        JSValue oldValue = elementAtIndex(index);
        elementAtIndex(index,element);
        return (T) oldValue.toJavaObject(mType);
    }

    /**
     * @see List#add(int, Object)
     * @since 0.1.0
     */
    @Override
    @SuppressWarnings("unchecked")
    public void add(final int index, final T element) {
        throw new UnsupportedOperationException();
    }

    /**
     * @see List#remove(int)
     * @since 0.1.0
     */
    @Override
    @SuppressWarnings("unchecked")
    public T remove(final int index) {
        throw new UnsupportedOperationException();
    }

    /**
     * @see List#indexOf(Object)
     * @since 0.1.0
     */
    @Override
    public int indexOf(final Object object) {
        ListIterator<T> listIterator = listIterator();
        while (listIterator.hasNext()) {
            if (listIterator.next().equals(object)) {
                return listIterator.nextIndex() - 1;
            }
        }
        return -1;
    }

    /**
     * @see List#lastIndexOf(Object)
     * @since 0.1.0
     */
    @Override
    public int lastIndexOf(final Object object) {
        ListIterator<T> listIterator = listIterator(size());
        while (listIterator.hasPrevious()) {
            if (listIterator.previous().equals(object)) {
                return listIterator.previousIndex() + 1;
            }
        }
        return -1;
    }

    /**
     * @see List#listIterator()
     * @since 0.1.0
     */
    @Override @NonNull
    public ListIterator<T> listIterator() {
        return listIterator(0);
    }

    /**
     * @see List#listIterator(int)
     * @since 0.1.0
     */
    @Override @NonNull
    public ListIterator<T> listIterator(final int index) {
        return new ArrayIterator(index);
    }

    /**
     * @see List#equals(Object)
     * @since 0.1.0
     */
    @Override
    public boolean equals(final Object other) {
        if (other == null) {
            return false;
        }
        if (!(other instanceof List<?>)) {
            return false;
        }
        List<?> otherList = (List<?>)other;
        if (size() != otherList.size()) {
            return false;
        }
        Iterator<T> iterator = iterator();
        Iterator<?> otherIterator = otherList.iterator();
        while (iterator.hasNext() && otherIterator.hasNext()) {
            T next = iterator.next();
            Object otherNext = otherIterator.next();
            if (!next.equals(otherNext)) {
                return false;
            }
        }
        return true;
    }

    /**
     * @see List#hashCode()
     * @since 0.1.0
     */
    @Override
    public int hashCode() {
        int hashCode = 1;
        for (T e : this) {
            hashCode = 31 * hashCode + (e == null ? 0 : e.hashCode());
        }
        return hashCode;
    }
}
